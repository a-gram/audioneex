import os

from _base import BaseTestCase
from audioneex import (Indexer, Database, Callbacks, AudioSourceFile)


RECFILES = ["rec1.ogg", "rec2.ogg", "rec3.ogg"]
AUFILES = list(map(lambda f: os.path.join(BaseTestCase.DATADIR, f), RECFILES))


class MyCallbacks(Callbacks):

    def __init__(self, asource, test):
        Callbacks.__init__(self)
        self.asource = asource
        self.test = test
    
    def on_audio_request(self, FID, nsamples):
        self.test.assertEqual(1, FID)
        self.test.assertGreater(nsamples, 0)
        self.asource.read(nsamples)
        return self.asource.buffer()


class CallbacksTestCase(BaseTestCase):
    
    def setUp(self):
        self.db = Database(self.DATADIR)
        self.db.open(Database.BUILD)
        self.cbk = MyCallbacks(AudioSourceFile(AUFILES[0]), self)
        self.idx = Indexer(self.db, self.cbk)
        self.idx.start()
    
    def test_on_audio_request(self):        
        self.idx.index(1)
    
    def tearDown(self):
        self.idx.end()
        self.db.close()
