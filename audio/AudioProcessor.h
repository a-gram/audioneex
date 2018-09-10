/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <cassert>
#include <vector>
#include <iostream>
#include <sstream>
#include <memory>

#ifdef HAVE_LIBRESAMPLE
 #include <libresample/libresample.h>
#endif

#include "AudioBlock.h"
#include "FFT.h"

/// Class implementing various audio processing routines on AudioBlocks.
/// The class is implicitly templatized on the audio sample format thru
/// the AudioBlocks.

template <typename T>
class AudioProcessor
{
    std::unique_ptr<FFT> mFFT;

 public:

    /// C-tors.
    AudioProcessor();

    /// D-tors. (not meant for inheritance for now)
    ~AudioProcessor();

    /// Mix down the audio to a mono channel. This method performs either an
    /// 'in-place' transformation or creates a new block, depending on the 'inplace'
    /// flag. The client is responsible for deallocating the newly created blocks.
    /// @param inplace Indicates whether to perform the transformation in-place
    /// or into a new audio block. If the audio block is already mono, the method
    /// does nothing and returns a null pointer.
    /// @return A pointer to the new audio block containing the mixed down channels,
    /// or a null pointer if mixed in-place or the audio is already mono.
    AudioBlock<T>* ToMono(AudioBlock<T> &block, bool inplace = true);

    /// Mix down the audio in the input block into the block provided by the caller.
    void ToMono(AudioBlock<T> &inBlock, AudioBlock<T> &outBlock);

    /// Deinterleave the specified channel of the given block into a new block.
    /// The client is responsible for deallocating the newly created blocks.
    /// If the audio block is not multichannel (mono), then the method does nothing
    /// and returns the given block.
    /// @param chan The channel to be interleaved (1=chan1, 2=chan2, ...).
    /// If this parameter is not provided, all channels will be deinterleaved.
    /// @return A vector with new audio blocks containing the deinterleaved channels.
    std::vector<AudioBlock<T>*> Deinterleave(AudioBlock<T> &block, size_t chan = 0);

	///
	void Mix(AudioBlock<T> &block1, AudioBlock<T> &block2, AudioBlock<T> &out);

#ifdef HAVE_LIBRESAMPLE
    /// Resample the given audio block to the specified sample rate into a new block.
    /// Audio must be mono channel.
    AudioBlock<float> *Resample(AudioBlock<T> &block, float newRate);

    /// Resample the given audio block to the specified sample rate into the specified
    /// audio block. Audio must be mono channel.
    void Resample(AudioBlock<T> &inBlock, AudioBlock<float> &outBlock, float newRate);
#endif

    /// Performs the DFT of the input block using FFT. Input block's size must be a
    /// power of 2 for efficiency. Return the FFT in the given vector of float.
    /// The type of FFT returned is specified in the parameter <type>.
    void FFT_Transform(AudioBlock<float> &inBlock, std::vector<float> &fft, int type=FFT::MagnitudeSpectrum);

    /// Set FFT params
    void SetFFT(FFT* fft);

 private:

    /// Perform downmixing.
    void DoMixToMono(T* ibuf, T* obuf, size_t datasize, size_t channels);

};


///////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// IMPLEMENTATION ///////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////


template <typename T>
AudioProcessor<T>::AudioProcessor()
{
}


template <typename T>
AudioProcessor<T>::~AudioProcessor()
{
}


template <typename T>
AudioBlock<T>* AudioProcessor<T>::ToMono(AudioBlock<T> &block, bool inplace)
{
    assert(!block.IsNull());

    AudioBlock<T>* pblock = nullptr;

    // audio block is already mono. return.
    if(block.Channels() == 1)
       return pblock;

    // compute new block size after downmixing
    int newsize = block.Size() / block.Channels();

    if(!inplace)
    {
        pblock = new AudioBlock<T>(newsize, block.SampleRate(), 1);

        DoMixToMono(block.Data(), pblock->Data(), block.Size(), block.Channels());
    }
    else{
        DoMixToMono(block.Data(), block.Data(), block.Size(), block.Channels());

        block.SetChannels(1);
        block.SetSize(newsize);
    }

    return pblock;
}


template <typename T>
void AudioProcessor<T>::ToMono(AudioBlock<T> &inBlock, AudioBlock<T> &outBlock)
{
    assert(!inBlock.IsNull());

    size_t newsize = inBlock.Size() / inBlock.Channels();

    if(outBlock.IsNull())
       outBlock.Create(newsize, inBlock.SampleRate(), 1);
    else{
       assert(outBlock.Capacity() >= newsize);
       outBlock.SetSize(newsize);
       outBlock.SetChannels(1);
    }

    DoMixToMono(inBlock.Data(), outBlock.Data(), inBlock.Size(), inBlock.Channels());
}


template <typename T>
void AudioProcessor<T>::DoMixToMono(T* ibuf, T* obuf, size_t datasize, size_t channels)
{
    for(size_t i=0, s=0; s<datasize; i++, s=i*channels)
    {
        float val = 0;

        for(size_t c=0; c<channels; c++)
            val += static_cast<float>(ibuf[s+c]) /
                   static_cast<float>(channels);

        obuf[i] = static_cast<T>(val);
    }
}



template <typename T>
std::vector<AudioBlock<T>*> AudioProcessor<T>::Deinterleave(AudioBlock<T> &block, size_t chan)
{
    assert(!block.IsNull());
    assert(0 <= chan && chan <= block.Channels());

    size_t chans   = block.Channels();
    size_t oldsize = block.Size();
    size_t newsize = oldsize / chans;
    size_t dchans  = chans;

    std::vector<AudioBlock<T>*> chans_list;

    // block is mono, nothing to deinterleave, return block itself.
    if(chans == 1){
       chans_list.push_back(&block);
       return chans_list;
    }

    // do deinterleaving
    for(size_t coff=0, c=0; c<dchans; c++)
    {
        AudioBlock<T> *pblock = new AudioBlock<T>(newsize, block.SampleRate(), 1);

        // If user has specified a channel, deinterleave that channel only,
        // else deinterleave all channels.
        if(chan > 0) { coff = chan;  dchans = 1; } else coff = c;

        for(size_t i=0, s=0; s < oldsize; i++, s=i*chans)
            pblock->Data()[i] = block.Data()[s+coff];

        chans_list.push_back(pblock);
    }

    return chans_list;
}


template<typename T>
void AudioProcessor<T>::Mix(AudioBlock<T> &block1, AudioBlock<T> &block2, AudioBlock<T> &out)
{
	assert(block1.SampleRate() == block2.SampleRate());
	assert(block2.SampleRate() == out.SampleRate());
	assert(block1.Channels() == block2.Channels());
	assert(block2.Channels() == out.Channels());

	size_t mixedSamples = std::min(block1.Size(), block2.Size());
	assert(out.Capacity() >= mixedSamples);
	float nChans = block1.Channels();

	for(size_t i=0; i<mixedSamples; i++)
		out.Data()[i] = static_cast<T>(
		               (static_cast<float>(block1.Data()[i]) +
                        static_cast<float>(block2.Data()[i])) / nChans );

}


#ifdef HAVE_LIBRESAMPLE

template <typename T>
AudioBlock<float> *AudioProcessor<T>::Resample(AudioBlock<T> &block, float newRate)
{
    assert(block.Channels() == 1);

    // conversion factor (out_rate/in_rate)
    double factor = newRate / block.SampleRate();
    // 0=fast resampling (for real-time), 1=high quality resampling (slower but better)
    int quality = 0;
    // flag indicating whether this is the last block of samples (1=true, 0=false)
    int lastFlag = 1;
    // # of samples in the input buffer used to perform the conversion
    int inBufferUsed;
    // # of samples in the out buffer produced by the conversion
    int outSamples;

    size_t inBufferLen = block.Size();
    size_t outBufferLen = inBufferLen * factor;

    AudioBlock<float>* pOutBlock = new AudioBlock<float>(outBufferLen + 64,
                                                         block.SampleRate(),
                                                         block.Channels());

    /// input block needs to be normalized in [-1,1]

    float *inBuffer = new float[inBufferLen];
    float *outBuffer = pOutBlock->Data();

    for(size_t i=0; i<inBufferLen; i++)
        inBuffer[i] = block[i] / 32768.0f;


    void *handle = resample_open(quality, factor, factor);

    outSamples = resample_process(handle, factor,
                                  inBuffer, inBufferLen,
                                  lastFlag, &inBufferUsed,
                                  outBuffer, outBufferLen);

    if(outSamples >= 0)
       pOutBlock->Resize(outSamples);
    else{
       std::cerr << "AudioProcessor::Resample() - Errors occurred during resampling." << std::endl;
       pOutBlock->Resize(0);
    }

//    std::cout << "Processed=" << inBufferUsed << "/" << inBufferLen
//              << " Produced=" << outSamples << "/" << outBufferLen
//              << std::endl;

    resample_close(handle);

    delete[] inBuffer;

    return pOutBlock;

}



template <typename T>
void AudioProcessor<T>::Resample(AudioBlock<T> &inBlock, AudioBlock<float> &outBlock, float newRate)
{
    assert(!inBlock.IsNull());
    assert(inBlock.Channels() == 1);

    // conversion factor (out_rate/in_rate)
    double factor = newRate / inBlock.SampleRate();
    // 0=fast resampling (for real-time), 1=high quality resampling (slower but better)
    int quality = 0;
    // flag indicating whether this is the last block of samples (1=true, 0=false)
    int lastFlag = 1;
    // # of samples in the input buffer used to perform the conversion
    int inBufferUsed;
    // # of samples in the out buffer produced by the conversion
    int outSamples;


    if(outBlock.IsNull())
       outBlock.Create(inBlock.Size() * factor + 64, newRate, 1);

    assert(outBlock.Capacity() > inBlock.Size() * factor);
    assert(outBlock.Channels() == 1);
    assert(outBlock.SampleRate() == newRate);

    // libresample needs the audio data in float format normalized in the range [-1,1],
    // so check whether the input block is already in this format and, if not, normalize it.

    AudioBlock<float> *normBlock = inBlock.Normalize();

    if(normBlock == nullptr){
        outBlock.Resize(0);
        std::cerr << "AudioProcessor::Resample() - AudioBlock::Normalize() returned 'null'" << std::endl;
        return;
    }


    float *inBuffer  = normBlock->Data();
    float *outBuffer = outBlock.Data();

    void *handle = resample_open(quality, factor, factor);

    outSamples = resample_process(handle, factor,
                                  inBuffer, normBlock->Size(),
                                  lastFlag, &inBufferUsed,
                                  outBuffer, outBlock.Capacity());// outBlock.Size());

    outBlock.Resize(outSamples);

//    std::cout << "Processed=" << inBufferUsed << "/" << normBlock->Size()
//              << " Produced=" << outSamples << "/" << outBlock.Size()
//              << std::endl;

    resample_close(handle);

    delete normBlock;

}

#endif

template <typename T>
void AudioProcessor<T>::FFT_Transform(AudioBlock<float> &inBlock, std::vector<float> &fft, int type)
{
    assert(mFFT.get());

    mFFT->Compute(inBlock);

    FFTFrame &fftFrame = mFFT->GetFFTFrame();

    if(fft.size()!=fftFrame.Size())
       fft.resize(fftFrame.Size());

    if(type == FFT::MagnitudeSpectrum)
    {
       for(size_t i=0; i<fft.size(); i++)
           fft[i] = fftFrame.Magnitude(i);
    }
    else if(type == FFT::PowerSpectrum)
    {
       for(size_t i=0; i<fft.size(); i++)
           fft[i] = fftFrame.Power(i);
    }
    else if(type == FFT::EnergySpectrum)
    {
       for(size_t i=0; i<fft.size(); i++)
           fft[i] = fftFrame.Energy(i);
    }

}


template <typename T>
void AudioProcessor<T>::SetFFT(FFT *fft)
{
    mFFT.reset(fft);
}


#endif // AUDIOPROCESSOR_H
