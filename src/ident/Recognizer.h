/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef RECOGNIZER_H
#define RECOGNIZER_H

#include "Matcher.h"

// The following classes are not part of the public API but we need
// their interfaces exposed when testing DLLs.
#ifdef TESTING
  #define AUDIONEEX_API_TEST AUDIONEEX_API
#else
  #define AUDIONEEX_API_TEST
#endif


namespace Audioneex
{

/// Identification accumulator/integrator
struct IdAcc_t
{
    float Conf  {0.f};
    float Time  {0.f};
    float Steps {0.f};
};

typedef boost::unordered::unordered_map<uint32_t, IdAcc_t> hashtable_acc;


/// Implementation of the Recognizer interface

class AUDIONEEX_API_TEST RecognizerImpl : public Audioneex::Recognizer
{
    AudioBlock<float>                 m_AudioBuffer;
    Fingerprint                       m_Fingerprint;
    Matcher                           m_Matcher;
    std::vector<Audioneex::IdMatch>   m_IdMatches;
    float                             m_BinaryIdThreshold;
    float                             m_BinaryIdMinTime;
    hashtable_acc                     m_MatchAcc;
    double                            m_IdTime;


    /// Process match results at each processing step. This method shall
    /// be called right after a Matcher::Process() call to analyze the
    /// current state of the identification.
    ///
    /// @param processed  Indicates whether the matcher has processed the
    ///                   audio (the matcher does not necessarily perform
    ///                   a processing at every step, see Matcher::Process()
    ///                   for more details). This is the value returned by
    ///                   Matcher::Process() and currently it is the number
    ///                   of LFs that have been processed.
    ///
    /// @param dt_proc    Indicates the processing step (in seconds), that
    ///                   is the length of the audio being processed.
    ///
    void ProcessMatchResults(int processed, float dt_proc);
	
    /// Create results data.
    void FillResults();

public:

    RecognizerImpl();

    void SetAudioBufferSize(float seconds);

    // Public interface (see audioneex.h)

    void       SetBinaryIdThreshold(float value);
    void       SetBinaryIdMinTime(float value);
    void       SetMaxRecordingDuration(size_t duration);
    void       SetDataStore(Audioneex::DataStore* dstore);
    float      GetBinaryIdThreshold() const { return m_BinaryIdThreshold; }
    float      GetBinaryIdMinTime() const { return m_BinaryIdMinTime; }
    DataStore* GetDataStore() const { return m_Matcher.GetDataStore(); }
    double     GetIdentificationTime() const { return m_IdTime; }
    void       Identify(const float *audio, size_t nsamples);
    Audioneex::IdMatch* GetResults();
    void       Flush();
    void       Reset();

};

}// end namespace Audioneex

#endif
