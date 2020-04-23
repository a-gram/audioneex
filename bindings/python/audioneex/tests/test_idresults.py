import re

from _base import BaseTestCase
from audioneex import IdResults, IdMatch


RX_OUT_OF_BOUND = re.compile("out of bound", re.IGNORECASE)


class IdResultsTestCase(BaseTestCase):
        
    def test___init__(self):        
        res = IdResults()
        self.assertEqual(0, len(res))
        
    def test___getitem__(self):
        res = IdResults()
        with self.assertRaisesRegex(RuntimeError, RX_OUT_OF_BOUND):
            r = res[0]
        
    def test___setitem__(self):
        res = IdResults()
        with self.assertRaisesRegex(RuntimeError, RX_OUT_OF_BOUND):
            res[0] = IdMatch()

    def test___len__(self):
        res = IdResults()
        self.assertEqual(0, len(res))
        
    def test___iter__(self):
        res = IdResults()
        it = iter(res)
        self.assertRaises(StopIteration, it.__next__)
        