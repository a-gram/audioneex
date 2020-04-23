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

#include "AudioBuffer.h"
#include "FFT.h"

/// Class implementing various audio processing routines on AudioBuffers
/// with audio format of type T.

template <typename T>
class AudioProcessor
{
    FFT mFFT;

 public:

    /// C-tors.
    AudioProcessor() = default;

    /// D-tors.
    ~AudioProcessor() = default;

	/// Mix two audio blocks into the given buffer
    void 
    Mix(AudioBuffer<T> &block1, 
        AudioBuffer<T> &block2, 
        AudioBuffer<T> &out)
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

    /// Perform the FFT of the input buffer. Input buffer's size must be a
    /// power of 2 for efficiency. Return the FFT in the given vector of float.
    /// The type of FFT returned is specified in the <type> parameter.
    void 
    FFT_Transform(AudioBuffer<float> &inBlock, 
                  std::vector<float> &fft, 
                  int type = FFT::MagnitudeSpectrum)
    {
        mFFT.Compute(inBlock);

        FFTFrame &fftFrame = mFFT.GetFFTFrame();

        if(fft.size() != fftFrame.Size())
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
        else
        {
           throw std::logic_error
           ("Bug: Unknown FFT type");
        }

    }
    
 private:


};

#endif // AUDIOPROCESSOR_H
