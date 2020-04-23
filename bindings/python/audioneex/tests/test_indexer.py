import os
import re

from _base import BaseTestCase
from audioneex import (Indexer, Database, AudioProvider, AudioSourceFile, 
                       K_MSCALE_MATCH, K_XSCALE_MATCH)


RECFILES = ["rec1.ogg", "rec2.ogg"]
AUFILES = list(map(lambda f: os.path.join(BaseTestCase.DATADIR, f), RECFILES))
MTYPES= [K_MSCALE_MATCH, K_XSCALE_MATCH]

RX_NO_DATASTORE = re.compile("no datastore set", re.IGNORECASE)
RX_NO_AUDIO_PROVIDER = re.compile("no audio provider set", re.IGNORECASE)
RX_MULTI_SESSIONS = re.compile("session .* already open", re.IGNORECASE)
RX_WRONG_DB_MODE = re.compile("invalid database mode", re.IGNORECASE)
RX_NO_SESSION = re.compile("no .* session open", re.IGNORECASE)
RX_NO_FINGERPRINT = re.compile(" no(t)? .* fingerprint", re.IGNORECASE)
RX_OUT_OF_BOUND = re.compile("out of bound", re.IGNORECASE)


class IndexerTestCase(BaseTestCase):
        
    def test___init__(self):        
        db = Database(self.DATADIR)
        ap = AudioProvider(AudioSourceFile())

        idx = Indexer()
        self.assertIsNone(idx.get_datastore())
        self.assertIsNone(idx.get_audio_provider())
        
        idx = Indexer(db, ap)
        self.assertIsNotNone(idx.get_datastore())
        self.assertIsNotNone(idx.get_audio_provider())
    
        self.assertIn(idx.get_match_type(), MTYPES)
        self.assertGreater(idx.get_cache_limit(), 0)
        self.assertEqual(0, idx.get_cache_used())
        
    def test_start(self):
        db = Database(self.DATADIR)
        db.open(Database.BUILD)
        ap = AudioProvider(AudioSourceFile())
        
        idx = Indexer()
        self.assertRaisesRegex(RuntimeError, RX_NO_DATASTORE, idx.start)
        idx.set_datastore(db)
        idx.start()
        self.assertRaisesRegex(RuntimeError, RX_MULTI_SESSIONS, idx.start)
        
        db.set_op_mode(Database.FETCH)
        idx = Indexer()
        idx.set_datastore(db)
        self.assertRaisesRegex(RuntimeError, RX_WRONG_DB_MODE, idx.start)
        
        db.set_op_mode(Database.BUILD)
        idx = Indexer(db, ap)
        idx.start()
        db.close()
        
    def test_index(self):
        db = Database(self.DATADIR)
        db.open(Database.BUILD)
        ap = AudioProvider(AudioSourceFile())

        idx = Indexer()
        idx.set_datastore(db)
        self.assertRaisesRegex(RuntimeError, RX_NO_SESSION, idx.index, 1)
        idx.start()
        self.assertRaisesRegex(RuntimeError, RX_NO_AUDIO_PROVIDER, idx.index, 1)
        idx.set_audio_provider(ap)
        self.assertEqual(0, idx.get_cache_used())
        self.assertEqual(0, db.get_fp_count())
        
        for fid, afile in enumerate(AUFILES):
            ap.audio_source.open(afile)
            idx.index(fid+1)
            
        self.assertRaisesRegex(RuntimeError, RX_NO_FINGERPRINT, idx.index, 3)
        self.assertGreater(idx.get_cache_used(), 0)
        self.assertEqual(2, db.get_fp_count())
        idx.end()
        db.clear()
        db.close()
        
    def test_end(self):
        db = Database(self.DATADIR)
        db.open(Database.BUILD)
        idx = Indexer(db, None)
        idx.end()  # no-op
        idx.start()
        idx.end()
        db.close()
        
    def test_set_match_type(self):
        idx = Indexer()
        idx.set_match_type(K_MSCALE_MATCH)
        self.assertEqual(K_MSCALE_MATCH, idx.get_match_type())
        idx.set_match_type(K_XSCALE_MATCH)
        self.assertEqual(K_XSCALE_MATCH, idx.get_match_type())
        
    def test_get_match_type(self):
        self.test_set_match_type()
        
    def test_set_cache_limit(self):
        idx = Indexer()
        idx.set_cache_limit(64)
        self.assertEqual(64, idx.get_cache_limit())
        idx.set_cache_limit(0)
        self.assertEqual(0, idx.get_cache_limit())
        self.assertRaisesRegex(RuntimeError, RX_OUT_OF_BOUND, 
                               idx.set_cache_limit, -1)

    def test_get_cache_limit(self):
        self.test_set_cache_limit()
        
    def test_get_cache_used(self):
        idx = Indexer()
        self.assertEqual(0, idx.get_cache_used())

    def test_set_datastore(self):
        db = Database(self.DATADIR)
        idx = Indexer()
        idx.set_datastore(db)
        self.assertIsNotNone(idx.get_datastore())
        
    def test_get_datastore(self):
        self.test_set_datastore()
        
    def test_set_audio_provider(self):
        ap = AudioProvider(AudioSourceFile())
        idx = Indexer()
        idx.set_audio_provider(ap)
        self.assertIsNotNone(idx.get_audio_provider())
        
    def test_get_audio_provider(self):
        self.test_set_audio_provider()

