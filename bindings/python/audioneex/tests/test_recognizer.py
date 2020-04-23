import os
import re

from _base import BaseTestCase
from audioneex import (Recognizer, Database, AudioBuffer, AudioSourceFile, 
                       K_MSCALE_MATCH, K_XSCALE_MATCH, K_FUZZY_IDENTIFICATION, 
                       K_BINARY_IDENTIFICATION, K_STRICT_IDENTIFICATION, 
                       K_EASY_IDENTIFICATION, K_UNIDENTIFIED, K_SOUNDS_LIKE, 
                       K_IDENTIFIED)


RECFILES = ["rec1.ogg", "rec2.ogg", "rec3.ogg"]
AUFILES = list(map(lambda f: os.path.join(BaseTestCase.DATADIR, f), RECFILES))
MTYPES = [K_MSCALE_MATCH, K_XSCALE_MATCH]
IDTYPES = [K_FUZZY_IDENTIFICATION, K_BINARY_IDENTIFICATION]
IDMODES = [K_STRICT_IDENTIFICATION, K_EASY_IDENTIFICATION]
IDCLASSES = [K_UNIDENTIFIED, K_SOUNDS_LIKE, K_IDENTIFIED]

RX_NO_DATASTORE = re.compile("no datastore set", re.IGNORECASE)


class RecognizerTestCase(BaseTestCase):
        
    def test___init__(self):        
        rec = Recognizer()
        self.assertIn(rec.get_match_type(), MTYPES)
        self.assertInRange([0, 1], rec.get_mms())
        self.assertIn(rec.get_identification_type(), IDTYPES)
        self.assertIn(rec.get_identification_mode(), IDMODES)
        self.assertInRange([0.5, 1], rec.get_binary_id_threshold())
        self.assertAlmostEqual(0, rec.get_binary_id_min_time())
        self.assertIsNone(rec.get_results())
        self.assertAlmostEqual(0, rec.get_identification_time())
        self.assertIsNone(rec.get_datastore())
        rec = Recognizer(Database())
        self.assertIn(rec.get_match_type(), MTYPES)
        self.assertInRange([0, 1], rec.get_mms())
        self.assertIn(rec.get_identification_type(), IDTYPES)
        self.assertIn(rec.get_identification_mode(), IDMODES)
        self.assertInRange([0.5, 1], rec.get_binary_id_threshold())
        self.assertAlmostEqual(0, rec.get_binary_id_min_time())
        self.assertIsNone(rec.get_results())
        self.assertAlmostEqual(0, rec.get_identification_time())
        self.assertIsNotNone(rec.get_datastore())

    def test_set_match_type(self):
        rec = Recognizer()
        rec.set_match_type(K_MSCALE_MATCH)
        self.assertEqual(K_MSCALE_MATCH, rec.get_match_type())
        self.assertRaises(RuntimeError, rec.set_match_type, -1)
        self.assertRaises(RuntimeError, rec.set_match_type, 2)
        
    def test_get_match_type(self):
        self.test_set_match_type()
        
    def test_set_mms(self):
        rec = Recognizer()
        rec.set_mms(0)
        self.assertEqual(0, rec.get_mms())
        rec.set_mms(1)
        self.assertEqual(1, rec.get_mms())
        self.assertRaises(RuntimeError, rec.set_mms, 2)
        self.assertRaises(RuntimeError, rec.set_mms, -1)

    def test_get_mms(self):
        self.test_set_mms()
        
    def test_set_identification_type(self):
        rec = Recognizer()
        rec.set_identification_type(K_FUZZY_IDENTIFICATION)
        self.assertEqual(K_FUZZY_IDENTIFICATION, rec.get_identification_type())
        rec.set_identification_type(K_BINARY_IDENTIFICATION)
        self.assertEqual(K_BINARY_IDENTIFICATION, rec.get_identification_type())
        self.assertRaises(RuntimeError, rec.set_identification_type, 2)
        self.assertRaises(RuntimeError, rec.set_identification_type, -1)

    def test_get_identification_type(self):
        self.test_set_identification_type()
    
    def test_set_identification_mode(self):
        rec = Recognizer()
        rec.set_identification_mode(K_STRICT_IDENTIFICATION)
        self.assertEqual(K_STRICT_IDENTIFICATION, rec.get_identification_mode())
        rec.set_identification_mode(K_EASY_IDENTIFICATION)
        self.assertEqual(K_EASY_IDENTIFICATION, rec.get_identification_mode())
        self.assertRaises(RuntimeError, rec.set_identification_mode, 2)
        self.assertRaises(RuntimeError, rec.set_identification_mode, -1)
        
    def test_get_identification_mode(self):
        self.test_set_identification_mode()
    
    def test_set_binary_id_threshold(self):
        rec = Recognizer()
        rec.set_binary_id_threshold(0.5)
        self.assertAlmostEqual(0.5, rec.get_binary_id_threshold())
        rec.set_binary_id_threshold(1)
        self.assertAlmostEqual(1, rec.get_binary_id_threshold())
        self.assertRaises(RuntimeError, rec.set_binary_id_threshold, 1.5)
        self.assertRaises(RuntimeError, rec.set_binary_id_threshold, -0.5)
        
    def test_get_binary_id_threshold(self):
        self.test_set_binary_id_threshold()
        
    def test_set_binary_id_min_time(self):
        rec = Recognizer()
        rec.set_binary_id_min_time(0)
        self.assertAlmostEqual(0, rec.get_binary_id_min_time())
        rec.set_binary_id_min_time(10.5)
        self.assertAlmostEqual(10.5, rec.get_binary_id_min_time())
        self.assertRaises(RuntimeError, rec.set_binary_id_min_time, 21)
        self.assertRaises(RuntimeError, rec.set_binary_id_min_time, -1)
        
    def test_get_binary_id_min_time(self):
        self.test_set_binary_id_min_time()
    
    def test_set_max_recording_duration(self):
        rec = Recognizer()
        rec.set_max_recording_duration(360)
        # NOTE: there is currently no way to verify the
        # effects of this method from the public interface
    
    def test_identify(self):
        db = Database(self.DATADIR, "db")
        db.open(Database.FETCH)
        rec = Recognizer()
        audio = AudioBuffer()
        src = AudioSourceFile()
        # Expected:
        self.assertRaisesRegex(RuntimeError, RX_NO_DATASTORE, 
                               rec.identify, audio)
        rec.set_datastore(db)
        
        # Identify fingerprinted audio
        def do_identify(randoff=False):
            for fileid in (1,2,3):
                offset = (fileid * 5) if randoff else 0
                src.set_position(offset)
                src.open(AUFILES[fileid-1])
                #print("Identifying rec%d @ %d"%(fileid, offset))
                while src.readin(audio) and rec.identify(audio) is None:
                    pass
                results = rec.get_results()
                # Expected: 
                self.assertEqual(1, len(results))
                self.assertEqual(fileid, results[0].FID)
                if (rec.get_identification_type() == K_BINARY_IDENTIFICATION and
                     rec.get_binary_id_min_time() > 1):
                    self.assertGreaterEqual(rec.get_identification_time(),
                                            rec.get_binary_id_min_time())
                src.close()
        
        # Identify with different types and modes
        rec.set_identification_type(K_BINARY_IDENTIFICATION)
        for th in [0.7, 0.8, 0.9]:
            rec.set_binary_id_threshold(th)
            do_identify(True)
        rec.set_identification_type(K_FUZZY_IDENTIFICATION)
        rec.set_identification_mode(K_EASY_IDENTIFICATION)
        do_identify()
        rec.set_identification_mode(K_STRICT_IDENTIFICATION)
        do_identify()
        # Verify the recognizer is automatically reset by identify()
        src.open(AUFILES[0])
        results = None
        while results is None and src.readin(audio):
            results = rec.identify(audio)
        self.assertEqual(1, len(results))
        self.assertInRange([3, 5], rec.get_identification_time())
        results = rec.get_results()
        self.assertEqual(1, len(results))
        self.assertInRange([3, 5], rec.get_identification_time())
        results = rec.identify(audio)
        self.assertIsNone(results)
        self.assertAlmostEqual(audio.duration(), rec.get_identification_time())
        results = rec.get_results()
        self.assertIsNone(results)
        self.assertAlmostEqual(audio.duration(), rec.get_identification_time())
        db.close()
        
    def test_get_results(self):
        db = Database(self.DATADIR, "db")
        db.open(Database.FETCH)
        rec = Recognizer(db)
        self.assertIsNone(rec.get_results())
        audio = AudioBuffer()
        rec.identify(audio)
        self.assertIsNone(rec.get_results())
        for idtype in IDTYPES:
            rec.set_identification_type(idtype)
            while rec.identify(audio) is None:
                pass
            self.assertEqual(0, len(rec.get_results()))
            self.assertInRange([15, 25], rec.get_identification_time())
        for idtype in IDTYPES:
            rec.set_identification_type(idtype)
            audio = AudioSourceFile(AUFILES[0])
            while audio.read() and rec.identify(audio.buffer()) is None:
                pass
            self.assertEqual(1, len(rec.get_results()))
            self.assertLessEqual(rec.get_identification_time(), 5)
        db.close()
            
    def test_set_datastore(self):
        db = Database(self.DATADIR, "db")
        rec = Recognizer()
        rec.set_datastore(db)
        self.assertIsNotNone(rec.get_datastore())
        
    def test_get_datastore(self):
        self.test_set_datastore()

