/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include <boost/unordered_map.hpp>
#include <list>
#include <vector>

#include "common.h"
#include "Parameters.h"
#include "AudioBlock.h"
#include "AudioProcessor.h"
#include "audioneex.h"

// The following classes are not part of the public API but we need
// their interfaces exposed when testing DLLs.
#ifdef TESTING
  #define AUDIONEEX_API_TEST AUDIONEEX_API
#else
  #define AUDIONEEX_API_TEST
#endif

namespace Audioneex
{

/// Raw Local Fingerprint structure
struct AUDIONEEX_API_TEST LocalFingerprint_t
{
    uint32_t  ID {0};
    uint32_t  T  {0};
    uint32_t  F  {0};
	
    std::vector<uint8_t> D;
};

/// Fingerprint structure
struct AUDIONEEX_API_TEST Fingerprint_t
{
    uint32_t ID {0};
	
    std::vector<LocalFingerprint_t> LFs;
};

/// Quantized Local Fingerprint structure
struct AUDIONEEX_API_TEST QLocalFingerprint_t
{
    uint32_t T;
    uint16_t F;
    uint8_t  W;
    uint8_t  E; // Clipped. See NOTE in Codebook::quantize()
};

// ----------------------------------------------------

typedef std::vector<LocalFingerprint_t>                       lf_vector;
typedef std::pair<LocalFingerprint_t*, LocalFingerprint_t*>   lf_pair;
typedef std::pair<QLocalFingerprint_t*, QLocalFingerprint_t*> qlf_pair;

inline bool operator ==(const LocalFingerprint_t &lf1,
                        const LocalFingerprint_t &lf2)
{
    return (lf1.ID == lf2.ID &&
            lf1.T == lf2.T &&
            lf1.F == lf2.F &&
            lf1.D == lf2.D);
}

// ----------------------------------------------------


class AUDIONEEX_API_TEST Fingerprint
{
    static const int POI_LOCATION = -1;

    AudioProcessor<int16_t>          m_AudioProcessor;
    AudioBlock<float>                m_OSBuffer;
    AudioBlock<float>                m_OSWindow;
    std::vector<std::vector<float> > m_Spectrum;
    std::vector<std::vector<float> > m_Peak;
    std::vector<float>               m_fftFrame;
    lf_vector                        m_LF;
    int                              m_LID;
    int                              m_DeltaT;

#ifdef PLOTTING_ENABLED
    std::vector<std::vector<float> > m_POI;  // For display purposes only
#endif

    void  ComputeSpectrum(AudioBlock<float> &audio, bool flush);
    void  FindPeaks();
    void  ExtractPOI();
    void  ComputeDescriptors();
    float ComputeWindowEnergy(int WoT, int WoF, int rWT, int rWF,
                              std::vector< std::vector<float> > &X);
    float ComputeMeanWindowEnergy(int WoT, int WoF, int rWT, int rWF,
                              std::vector< std::vector<float> > &X);

    friend class Tester;

 public:

    /// Construct a fingerprinter with a specified buffer size
    /// (in samples). The default size can hold 2 sec of audio.
    Fingerprint(size_t bufferSize = Pms::Fs * 2 + Pms::OrigWindowSize);
   ~Fingerprint() = default;

    /// Extract descriptors from the given audio block.
    /// The audio must be normalized in [-1,1], mono, 11025 Hz,
    /// and its max duration must be within the set buffer limit
    /// to avoid reallocation, while the min duration must be long
    /// enough to compute a sufficient amount of LF (currently
    /// at least 0.5 sec of audio).
    /// The 'flush' parameter indicates whether to process
    /// any residual audio data in the O&S buffer.
    /// This is usually done when the block is the last data
    /// piece in a stream of known length (i.e. a file).
    /// The max amount of residual audio equals the size of the
    /// O&S window (about 93 ms with the current settings).
    void Process(AudioBlock<float> &audio, bool flush=false);

    /// Reset the fingerprinter.
    void Reset();

    /// Get the Local Fingerprint stream produced at the last processing
    /// step. Must be called after Process().
    /// @note The produced local fingerprints are NOT owned by the
    ///       fingerprinter and MUST be retrieved and deleted by clients.
    const lf_vector& Get() const { return m_LF; }

    /// Set the internal buffer size. Use this method to properly
    /// set the audio buffer according to the expected audio
    /// snippets length to avoid reallocations. The default size can
    /// hold about 2 seconds of audio. The size is in samples.
    void SetBufferSize(size_t size);

    /// Get the size in samples of the internal audio buffer.
    size_t GetBufferSize() const { return m_OSBuffer.Capacity(); }

	/// Get the time delta (time-translation) so far processed
	int GetTimeDelta() const { return m_DeltaT; }
};


}// end namespace Audioneex

#endif // FINGERPRINT_H
