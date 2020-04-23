import unittest
import os
import glob


def get_data_dir():
    return os.path.join(os.path.dirname(os.path.realpath(__file__)), "data")

def isnumber(val):
    return (isinstance(val, int) or
            isinstance(val, float))
            
def isrange(val):
    return ((isinstance(val, tuple) or 
             isinstance(val, list)) and
             len(val) == 2)


class BaseTestCase(unittest.TestCase):

    DATADIR = get_data_dir()
    
    @classmethod
    def setUpClass(cls):
        if not os.path.exists(cls.DATADIR):
            raise RuntimeError(
                "Can't run tests. Data folder not found."
            )
        for f in glob.glob(os.path.join(cls.DATADIR, "data.*")):
            os.remove(f)

    @classmethod
    def tearDownClass(cls):
        pass
        
    def assertInRange(self, arange, val):
        if (not isrange(arange) or 
            not all(map(lambda v: isnumber(v), arange)) or
             not arange[0] < arange[1]):
            raise TypeError(
                "'{}' is not a valid numeric range. Must be a tuple (min, max)."
                .format(arange)
            )
        if not isnumber(val):
            raise TypeError(
                "'{}' is not a number".format(val)
            )
        if not arange[0] <= val <= arange[1]:
            raise AssertionError(
                "{} not in range {}".format(val, range)
            )

