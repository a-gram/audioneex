/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef RECOGNIZER_H
#define RECOGNIZER_H

#include "Matcher.h"
#include "MatchFuzzyClassifier.h"

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
    AudioBuffer<float>
    m_AudioBuffer;
    
    Fingerprinter
    m_Fingerprint;
    
    Matcher
    m_Matcher;
    
    MatchFuzzyClassifier
    m_Classifier;
    
    std::vector<Audioneex::IdMatch>
    m_IdMatches;
    
    Audioneex::eIdentificationType
    m_IdType;
    
    Audioneex::eIdentificationMode
    m_IdMode;
    
    float
    m_BinaryIdThreshold;
    
	float
    m_BinaryIdMinTime;
    
    hashtable_acc
    m_MatchAcc;
    
    double
    m_IdTime;


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
    void 
    ProcessMatchResults(int processed, float dt_proc);
    
    /// Do classification of best matches at current step.
    int 
    DoClassification(float Hu, float dT);
    
    /// Create results data.
    void 
    FillResults(int cresult=0);
    
    /// Reset the recognizer's internal state
    void
    Reset();
    
    
public:

    RecognizerImpl();

    void 
    SetAudioBufferSize(float seconds);

    // Public interface (see audioneex.h)

    void
    SetMatchType(Audioneex::eMatchType type);
    
    void
    SetMMS(float value);
    
    void
    SetIdentificationType(Audioneex::eIdentificationType type);
    
    void
    SetIdentificationMode(Audioneex::eIdentificationMode mode);
    
    void
    SetBinaryIdThreshold(float value);
    
	void
    SetBinaryIdMinTime(float value);
    
    void
    SetMaxRecordingDuration(size_t duration);
    
    void
    SetDataStore(Audioneex::DataStore* dstore);

    eMatchType 
    GetMatchType() const 
    { 
        return m_Matcher.GetMatchType();
    }
    
    float
    GetMMS() const 
    { 
        return m_Matcher.GetRerankThreshold();
    }
    
    eIdentificationType 
    GetIdentificationType() const 
    { 
        return m_IdType;
    }
    
    eIdentificationMode 
    GetIdentificationMode() const 
    { 
        return m_IdMode; 
    }
    
    float
    GetBinaryIdThreshold() const 
    { 
        return m_BinaryIdThreshold;
    }
    
	float
    GetBinaryIdMinTime() const 
    { 
        return m_BinaryIdMinTime;
    }
    
    DataStore* 
    GetDataStore() const 
    { 
        return m_Matcher.GetDataStore();
    }
    
    double
    GetIdentificationTime() const 
    { 
        return m_IdTime;
    }
    
    const Audioneex::IdMatch*
    Identify(const float *audio, size_t nsamples);
    
    const Audioneex::IdMatch* 
    GetResults();
    
    void
    Flush();
    
};

}// end namespace Audioneex

#endif
