import re

from _base import BaseTestCase
from audioneex import Database


DBOPS = [Database.BUILD, Database.FETCH]

RX_NOT_OPEN = re.compile("not open", re.IGNORECASE)
RX_OUT_OF_BOUND = re.compile("out of bound", re.IGNORECASE)
RX_INVALID_OPERATION = re.compile("invalid operation", re.IGNORECASE)
RX_INVALID_PARAMETER = re.compile("invalid parameter", re.IGNORECASE)


class DatabaseTestCase(BaseTestCase):
        
    def setUp(self):
        self.db = Database(self.DATADIR)
        
    def tearDown(self):
        self.db.close()
        
    def test___init__(self):
        db = Database()
        self.assertEqual("", db.get_url())
        self.assertEqual("data", db.get_name())
        self.assertFalse(db.is_open())
        db = Database("/some/path/to/db")
        self.assertEqual("/some/path/to/db", db.get_url())
        self.assertEqual("data", db.get_name())
        self.assertFalse(db.is_open())
        db = Database("/some/path/to/db", "the_name")
        self.assertEqual("/some/path/to/db", db.get_url())
        self.assertEqual("the_name", db.get_name())
        self.assertIsNone(db.close()) # no-op
        self.assertRaisesRegex(RuntimeError, RX_NOT_OPEN, db.is_empty)
        #self.assertRaisesRegex(RuntimeError, RX_NOT_OPEN, db.clear)
        self.assertFalse(db.is_open())
        self.assertRaisesRegex(RuntimeError, RX_NOT_OPEN, db.get_fp_count)
        self.assertRaisesRegex(RuntimeError, RX_NOT_OPEN, db.put_metadata, 1, "")
        self.assertRaisesRegex(RuntimeError, RX_NOT_OPEN, db.get_metadata, 1)
        self.assertRaisesRegex(RuntimeError, RX_NOT_OPEN, db.put_info, "")
        self.assertRaisesRegex(RuntimeError, RX_NOT_OPEN, db.get_info)
        self.assertIn(db.get_op_mode(), DBOPS)
        db = Database(self.DATADIR, "", "write")
        self.assertEqual(self.DATADIR, db.get_url())
        self.assertEqual("data", db.get_name())
        self.assertTrue(db.is_open())
        self.assertEqual(Database.BUILD, db.get_op_mode())
        db.close()
        db = Database(self.DATADIR, "", "read")
        self.assertEqual(self.DATADIR, db.get_url())
        self.assertEqual("data", db.get_name())
        self.assertTrue(db.is_open())
        self.assertEqual(Database.FETCH, db.get_op_mode())
        with self.assertRaisesRegex(RuntimeError, RX_INVALID_PARAMETER):
            db = Database(self.DATADIR, "", "cread")
        
    def test_open(self):
        self.db.open(Database.BUILD)
        self.assertTrue(self.db.is_open())
        self.assertEqual(Database.BUILD, self.db.get_op_mode())
        self.db.open(Database.FETCH)
        self.assertTrue(self.db.is_open())
        self.assertEqual(Database.FETCH, self.db.get_op_mode())
        self.assertRaises(RuntimeError, self.db.open, 5)
        
    def test_close(self):
        self.assertFalse(self.db.is_open())
        self.db.open(Database.BUILD)
        self.assertTrue(self.db.is_open())
        self.db.close()
        self.assertFalse(self.db.is_open())
        self.db.open(Database.FETCH)
        self.assertTrue(self.db.is_open())
        self.db.close()
        self.assertFalse(self.db.is_open())
        for i in range(10):
            self.db.close()  # no-op
        
    def test_is_empty(self):
        self.assertRaisesRegex(RuntimeError, RX_NOT_OPEN, self.db.is_empty)
        self.db.open(Database.BUILD)
        self.db.clear()
        self.assertTrue(self.db.is_empty())
        self.db.put_metadata(1, "test")
        self.assertFalse(self.db.is_empty())
        
    def test_clear(self):
        self.db.open(Database.BUILD)
        self.db.clear()
        self.assertTrue(self.db.is_empty())
        for i in range(1,10):
            self.db.put_metadata(i, "test %d" % i)
        self.assertFalse(self.db.is_empty())
        self.db.clear()
        self.assertTrue(self.db.is_empty())
        
    def test_is_open(self):
        self.assertFalse(self.db.is_open())
        self.db.open(Database.BUILD)
        self.assertTrue(self.db.is_open())
        self.db.close()
        self.assertFalse(self.db.is_open())
        self.db.open(Database.FETCH)
        self.assertTrue(self.db.is_open())
        self.db.close()
        self.assertFalse(self.db.is_open())
        
    def test_get_fp_count(self):
        # NOTE: we can't manually add fingerprints to the db
        #       so we are limited in what can be tested here
        self.assertRaisesRegex(RuntimeError, RX_NOT_OPEN, self.db.get_fp_count)
        self.db.open(Database.FETCH)
        self.assertEqual(0, self.db.get_fp_count())
        
    def test_put_metadata(self):
        self.db.open(Database.BUILD)
        for i in range(1,10):
            self.db.put_metadata(i, "test %d" % i)
        for i in range(1,10):
            self.assertEqual("test %d" % i, self.db.get_metadata(i))
        for i in range(1,10):
            self.db.put_metadata(i, "")
        for i in range(1,10):
            self.assertEqual("", self.db.get_metadata(i))
        self.assertRaises(Exception, self.db.put_metadata, -1, "test")
        self.assertRaises(Exception, self.db.put_metadata, int(5e9), "test")
        self.db.open(Database.FETCH)
        self.assertRaisesRegex(RuntimeError, RX_INVALID_OPERATION, 
                               self.db.put_metadata, 1, "test")
        
    def test_get_metadata(self):
        self.test_put_metadata()
        
    def test_put_info(self):
        self.db.open(Database.BUILD)
        self.db.put_info("test info")
        self.assertEqual("test info", self.db.get_info())
        self.db.put_info("")
        self.assertEqual("", self.db.get_info())
        self.db.open(Database.FETCH)
        self.assertRaisesRegex(RuntimeError, RX_INVALID_OPERATION, 
                               self.db.put_info, "test")

    def test_get_info(self):
        self.test_put_info()

    def test_get_op_mode(self):
        self.db.set_op_mode(Database.BUILD)
        self.assertEqual(Database.BUILD, self.db.get_op_mode())
        self.db.set_op_mode(Database.FETCH)
        self.assertEqual(Database.FETCH, self.db.get_op_mode())
        self.assertRaises(RuntimeError, self.db.set_op_mode, 5)

    def test_set_op_mode(self):
        self.test_get_op_mode()

    def test_set_url(self):
        self.db.set_url("/some/path/to/db")
        self.assertEqual("/some/path/to/db", self.db.get_url())

    def test_get_url(self):
        self.test_set_url()

    def test_set_name(self):
        self.db.set_name("the_name")
        self.assertEqual("the_name", self.db.get_name())

    def test_get_name(self):
        self.test_set_name()

