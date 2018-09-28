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

#include "AudioBlock.h"
#include "FFT.h"

/// Class implementing various audio processing routines on AudioBlocks
/// with audio format of type T.

template <typename T>
class AudioProcessor
{
    std::unique_ptr<FFT> mFFT;

 public:

    /// C-tors.
    AudioProcessor() = default;

    /// D-tors.
    ~AudioProcessor() = default;

	/// Mix two audio blocks into the given buffer
    void Mix(AudioBlock<T> &block1, AudioBlock<T> &block2, AudioBlock<T> &out)
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

    /// Performs the DFT of the input block using FFT. Input block's size must be a
    /// power of 2 for efficiency. Return the FFT in the given vector of float.
    /// The type of FFT returned is specified in the <type> parameter.
    void FFT_Transform(AudioBlock<float> &inBlock, 
                       std::vector<float> &fft, 
                       int type = FFT::MagnitudeSpectrum)
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

    /// Set FFT params
    void SetFFT(FFT *fft)
    {
        mFFT.reset(fft);
    }

 private:


};

#endif // AUDIOPROCESSOR_H
