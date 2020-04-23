import re

from _base import BaseTestCase
from audioneex import AudioBuffer


AUDIO_SRATE = 11025
AUDIO_CHANNELS = 1
AUDIO_BPS = 32

RX_OUT_OF_BOUND = re.compile("out of bound", re.IGNORECASE)


class AudioBufferTestCase(BaseTestCase):
    
    def test___init__(self):
        buffer = AudioBuffer()
        self.assertGreater(buffer.size(), 0)
        self.assertEqual(buffer.size(), buffer.capacity())
        self.assertEqual(buffer.sample_rate(), AUDIO_SRATE)
        self.assertEqual(buffer.channels(), AUDIO_CHANNELS)
        self.assertEqual(buffer.bits_per_sample(), AUDIO_BPS)
        self.assertAlmostEqual(buffer.duration(), 1.5, 3)
        buffer = AudioBuffer(11025)
        self.assertEqual(buffer.size(), 11025)
        self.assertEqual(buffer.size(), buffer.capacity())
        self.assertEqual(buffer.sample_rate(), AUDIO_SRATE)
        self.assertEqual(buffer.channels(), AUDIO_CHANNELS)
        self.assertEqual(buffer.bits_per_sample(), AUDIO_BPS)
        self.assertEqual(buffer.duration(), 1)
        buffer = AudioBuffer(0)
        self.assertEqual(buffer.size(), 0)
        self.assertEqual(buffer.size(), buffer.capacity())
        self.assertEqual(buffer.sample_rate(), AUDIO_SRATE)
        self.assertEqual(buffer.channels(), AUDIO_CHANNELS)
        self.assertEqual(buffer.bits_per_sample(), AUDIO_BPS)
        self.assertEqual(buffer.duration(), 0)
        with self.assertRaisesRegex(RuntimeError, RX_OUT_OF_BOUND):
            buffer = AudioBuffer(-5)


    def test___setitem__(self):
        buffer = AudioBuffer()
        for i in range(len(buffer)):
            buffer[i] = i
            self.assertEqual(buffer[i], i)
        with self.assertRaisesRegex(RuntimeError, RX_OUT_OF_BOUND):
            n = buffer[-1]
        with self.assertRaisesRegex(RuntimeError, RX_OUT_OF_BOUND):
            n = buffer[99999]
        with self.assertRaisesRegex(RuntimeError, RX_OUT_OF_BOUND):
            buffer[-1] = 0
        with self.assertRaisesRegex(RuntimeError, RX_OUT_OF_BOUND):
            buffer[99999] = 0
    
    def test___getitem__(self):
        self.test___setitem__()
    
    def test___len__(self):
        buffer = AudioBuffer(1024)
        self.assertEqual(len(buffer), 1024)
        self.assertEqual(len(buffer), buffer.size())
        
    def test___iter__(self):
        buffer = AudioBuffer(3)
        for i in range(len(buffer)):
            buffer[i] = i
        it = iter(buffer)
        self.assertEqual(it.__next__(), 0)
        self.assertEqual(it.__next__(), 1)
        self.assertEqual(it.__next__(), 2)
        self.assertRaises(StopIteration, it.__next__)
        l = [n for n in buffer]
        self.assertListEqual(l, [0, 1, 2])
        for i, n in enumerate(buffer):
            self.assertEqual(buffer[i], n)
    
    def test_capacity(self):
        buffer = AudioBuffer(1024)
        self.assertGreaterEqual(buffer.capacity(), 1024)
        buffer = AudioBuffer(0)
        self.assertGreaterEqual(buffer.capacity(), 0)
        
    def test_size(self):
        buffer = AudioBuffer(1024)
        self.assertEqual(buffer.size(), 1024)
        buffer = AudioBuffer(0)
        self.assertEqual(buffer.size(), 0)
        buffer.set_size(100)
        self.assertEqual(buffer.size(), 100)
        
    def test_set_size(self):
        buffer = AudioBuffer(1024)
        self.assertEqual(buffer.size(), 1024)
        buffer.set_size(100)
        self.assertEqual(buffer.size(), 100)
        buffer.set_size(0)
        self.assertEqual(buffer.size(), 0)
        self.assertRaisesRegex(RuntimeError, RX_OUT_OF_BOUND, 
                               buffer.set_size, -100)
        
    def test_sample_rate(self):
        buffer = AudioBuffer()
        self.assertEqual(buffer.sample_rate(), AUDIO_SRATE)
        
    def test_channels(self):
        buffer = AudioBuffer()
        self.assertEqual(buffer.channels(), AUDIO_CHANNELS)

    def test_bits_per_sample(self):
        buffer = AudioBuffer()
        self.assertEqual(buffer.bits_per_sample(), AUDIO_BPS)

    def test_duration(self):
        buffer = AudioBuffer(11025)
        self.assertEqual(buffer.duration(), 1)
        buffer = AudioBuffer(0)
        self.assertEqual(buffer.duration(), 0)
        
    def test_copy_to(self):
        buffer1 = AudioBuffer(100)
        buffer2 = AudioBuffer(100)
        for i in range(len(buffer1)):
            buffer1[i] = i
        buffer1.copy_to(buffer2)
        self.assertEqual(buffer1.size(), 100)
        self.assertEqual(buffer2.size(), 100)
        for n1, n2 in zip(buffer1, buffer2):
            self.assertEqual(n1, n2)
        buffer3 = AudioBuffer(50)
        buffer1.copy_to(buffer3)
        self.assertEqual(buffer1.size(), 100)
        self.assertEqual(buffer3.size(), 100)
        for n1, n2 in zip(buffer1, buffer3):
            self.assertEqual(n1, n2)
        buffer4 = AudioBuffer(0)
        buffer4.copy_to(buffer1)
        self.assertEqual(buffer1.size(), 0)
        self.assertEqual(buffer4.size(), 0)
        buffer1.copy_to(buffer4)
        self.assertEqual(buffer1.size(), 0)
        self.assertEqual(buffer4.size(), 0)

        
