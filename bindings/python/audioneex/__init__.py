
from .audioneex import *


class AudioProvider(Callbacks):
    """
    A default implementation of the engine audio callbacks.
    
    This class provides a default implementation of the callbacks
    invoked by the recognition system that can be used out of the box.
    You can derive your own implementations from the Callbacks base class.
    
    NOTE
    ----    
    Exceptions occurring within these callbacks are wrapped into a generic
    error by the wrapper. If you receive "Errors detected when calling ..."
    one of the callbacks, start debugging from there.
    
    """
    def __init__(self, asource=None):
        """
        Initialize the class
            
        Paramaters
        ----------
        asource: AudioSource
            An audio source from which to read audio samples
            
        """
        Callbacks.__init__(self)  # ALWAYS call the base c'tor
        self.audio_source = asource
    
    def on_audio_request(self, FID, nsamples):
        """
        Called during the fingerprinting process to request audio data
        
        Paramaters
        ----------
        FID: int
            The fingerprint id of the recording being processed
        nsamples: int
            The number of samples requested by the engine. Audio must
            be mono 11025Hz normalized in [-1,1].
        
        Returns
        -------
        AudioBuffer, None
            An audio buffer containing the audio samples. Note that
            although the number of read samples should be equal to the 
            requested amount, they can be less if there is not enough
            audio available. If any issues occur, the method must return
            nothing (i.e. None).
        
        """
        if self.audio_source is None:
            return None
        self.audio_source.read(nsamples)
        return self.audio_source.buffer()

