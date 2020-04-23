import os
import re

from _base import BaseTestCase
from audioneex import AudioSourceFile, AudioBuffer


AUDIO_SRATE = 11025
AUDIO_CHANNELS = 1
AUDIO_BPS = 32
READSIZE=16538

RECFILES = ["rec1.ogg", "rec2.ogg"]
AUFILES = list(map(lambda f: os.path.join(BaseTestCase.DATADIR, f), RECFILES))

RX_OUT_OF_BOUND = re.compile("out of bound", re.IGNORECASE)
RX_CAPACITY_EXCEEDED = re.compile("capacity exceeded", re.IGNORECASE)
RX_NULL_POINTER = re.compile("null pointer", re.IGNORECASE)
RX_NEGATIVE_VALUE = re.compile("negative value", re.IGNORECASE)


class AudioSourceFileTestCase(BaseTestCase):
    
    def test___init__(self):
        asource = AudioSourceFile()
        self.assertEqual("", asource.filename())
        self.assertFalse(asource.is_open())
        self.assertTrue(asource.buffer().size() > 0)
        asource = AudioSourceFile(AUFILES[0])
        self.assertEqual(AUFILES[0], asource.filename())
        self.assertTrue(asource.is_open())
        self.assertTrue(asource.buffer().size() > 0)
        #with self.assertRaises(RuntimeError):
        #    asource = AudioSourceFile("/not/existent/file")
    
    def test_open(self):
        asource = AudioSourceFile()
        self.assertFalse(asource.is_open())
        asource.open(AUFILES[0])
        self.assertTrue(asource.is_open())
        self.assertEqual(AUFILES[0], asource.filename())
        asource.open(AUFILES[1])
        self.assertTrue(asource.is_open())
        self.assertEqual(AUFILES[1], asource.filename())
        #with self.assertRaises(RuntimeError):
        #    asource.open("/not/existent/file")
        
    def test_close(self):
        asource = AudioSourceFile(AUFILES[0])
        self.assertTrue(asource.is_open())
        asource.close()
        self.assertFalse(asource.is_open())
        asource.close()  # no-op
    
    def test_is_open(self):
        asource = AudioSourceFile()
        self.assertFalse(asource.is_open())
        asource = AudioSourceFile(AUFILES[0])
        self.assertTrue(asource.is_open())
        asource.close()
        self.assertFalse(asource.is_open())
        asource.open(AUFILES[0])
        self.assertTrue(asource.is_open())
        asource.open(AUFILES[1])
        self.assertTrue(asource.is_open())
    
    def test_read(self):
        asource = AudioSourceFile(AUFILES[0])
        self.assertEqual(READSIZE, asource.read())
        self.assertEqual(READSIZE, asource.buffer().size())
        self.assertGreater(sum(map(lambda x: x*x, asource.buffer())), 1)
        self.assertEqual(11025, asource.read(11025))
        self.assertEqual(11025, asource.buffer().size())
        self.assertGreater(sum(map(lambda x: x*x, asource.buffer())), 1)
        self.assertEqual(0, asource.read(0))
        self.assertEqual(0, asource.buffer().size())
        asource = AudioSourceFile()
        asource.set_position(15)
        asource.open(AUFILES[1])
        nsamples = asource.buffer().size()
        self.assertEqual(nsamples, asource.read(nsamples))
        self.assertEqual(nsamples, asource.buffer().size())
        self.assertLess(asource.read(nsamples), nsamples)
        self.assertLess(asource.buffer().size(), nsamples)
        asource.open(AUFILES[0])
        with self.assertRaisesRegex(RuntimeError, RX_CAPACITY_EXCEEDED):
            asource.read(AUDIO_SRATE * 20)
        
    
    def test_readin(self):
        asource = AudioSourceFile(AUFILES[0])
        buffer = AudioBuffer()
        self.assertEqual(buffer.size(), asource.readin(buffer))
        self.assertGreater(sum(map(lambda x: x*x, buffer)), 1)
        buffer.set_size(0)
        self.assertEqual(0, asource.readin(buffer))
        buffer.set_size(11025)
        with self.assertRaisesRegex(RuntimeError, RX_NULL_POINTER):
            asource.readin(None)
        
    def test_filename(self):
        asource = AudioSourceFile()
        self.assertEqual("", asource.filename())
        asource = AudioSourceFile(AUFILES[0])
        self.assertEqual(AUFILES[0], asource.filename())
        asource.open(AUFILES[1])
        self.assertEqual(AUFILES[1], asource.filename())
        
    def test_set_position(self):
        asource = AudioSourceFile(AUFILES[0])
        asource.set_position(10)
        self.assertEqual(10, asource.get_position())
        asource.set_position(0)
        self.assertAlmostEqual(asource.get_position(), 0)
        self.assertRaisesRegex(RuntimeError, RX_NEGATIVE_VALUE,
                               asource.set_position, -10)
        
    def test_get_position(self):
        self.test_set_position()
    

