/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#include <cmath>
#include <climits>

#include "common.h"
#include "Recognizer.h"
#include "audioneex.h"

#ifdef TESTING
 #include "Tester.h"
#endif

TEST_HERE( namespace { Audioneex::Tester TEST; } )


/// Version string
const char* Audioneex::GetVersion() { return ENGINE_VERSION_STR; }

//=============================================================================
//                               Recognizer
//=============================================================================

Audioneex::Recognizer* Audioneex::Recognizer::Create() {
    return new RecognizerImpl;
}

//=============================================================================
//                              RecognizerImpl
//=============================================================================

Audioneex::RecognizerImpl::RecognizerImpl() :
    m_AudioBuffer          (Pms::Fs * 2.5 * Pms::Ca, Pms::Fs, Pms::Ca, 0),
    m_Fingerprint          (Pms::Fs * 2.5 * Pms::Ca),
    m_IdType               (FUZZY_IDENTIFICATION),
    m_IdMode               (EASY_IDENTIFICATION),
    m_BinaryIdThreshold    (0.9),
	m_BinaryIdMinTime      (0.f),
    m_IdTime               (0.0)
{
}

// ----------------------------------------------------------------------------

void Audioneex::RecognizerImpl::SetMatchType(eMatchType type)
{
    if(type != MSCALE_MATCH && type != XSCALE_MATCH)
       throw Audioneex::InvalidParameterException("Invalid match type set");

    m_Matcher.SetMatchType(type);
}

// ----------------------------------------------------------------------------

void Audioneex::RecognizerImpl::SetMMS(float value)
{
    if(value<0 || value>1)
       throw Audioneex::InvalidParameterException("Invalid MMS. Must be in [0,1]");

    m_Matcher.SetRerankThreshold(value);
}

// ----------------------------------------------------------------------------

void Audioneex::RecognizerImpl::SetIdentificationType(eIdentificationType type)
{
    if(type != BINARY_IDENTIFICATION && type != FUZZY_IDENTIFICATION)
       throw Audioneex::InvalidParameterException("Invalid identification type set");

    m_IdType = type;
}

// ----------------------------------------------------------------------------

void Audioneex::RecognizerImpl::SetIdentificationMode(eIdentificationMode mode)
{
    if(mode != STRICT_IDENTIFICATION && mode != EASY_IDENTIFICATION)
       throw Audioneex::InvalidParameterException("Invalid identification mode set");

    m_IdMode = mode;
}

// ----------------------------------------------------------------------------

void Audioneex::RecognizerImpl::SetBinaryIdThreshold(float value)
{
    if(value<0.5 || value>1)
       throw Audioneex::InvalidParameterException("Invalid binary id threshold. Must be in [0.5,1]");

    m_BinaryIdThreshold = value;
}

// ----------------------------------------------------------------------------

void Audioneex::RecognizerImpl::SetBinaryIdMinTime(float value)
{
	if(value<0 || value>Pms::MaxIdTime)
       throw Audioneex::InvalidParameterException("Invalid binary id min time. Must be in [0,20]");

	m_BinaryIdMinTime = value;
}

// ----------------------------------------------------------------------------

void Audioneex::RecognizerImpl::SetMaxRecordingDuration(size_t duration)
{
    m_Matcher.SetMaxRecordingDuration(duration);
}

// ----------------------------------------------------------------------------

void Audioneex::RecognizerImpl::Identify(const float *audio, size_t nsamples)
{
    if(audio == nullptr)
       throw Audioneex::InvalidParameterException("Got null audio pointer");

    // Nothing to identify
    if(nsamples == 0)
       return;

    // Any audio exceeding the internal buffer capacity will be discarded.
    // This limits the length of the audio snippets that the recognizer
    // can accept to the internal buffer length.
    if(m_AudioBuffer.Capacity() < nsamples)
       WARNING_MSG("Buffer overflow. Data truncation will occur.")

    m_AudioBuffer.SetData(audio, nsamples);
    m_IdTime += m_AudioBuffer.Duration();

    m_Fingerprint.Process(m_AudioBuffer);
    const lf_vector &lfs = m_Fingerprint.Get();
    int processed = m_Matcher.Process(lfs);

    // Process match results, if any (see Match::Process())
    ProcessMatchResults( processed, m_AudioBuffer.Duration() );

    m_AudioBuffer.Resize(0);
}

// ----------------------------------------------------------------------------

void Audioneex::RecognizerImpl::ProcessMatchResults(int processed, float dt_proc)
{
    // Return if identification results have already been produced
    if(!m_IdMatches.empty())
       return;

    const MatchResults_t &mresults = m_Matcher.GetResults();

    TEST_HERE( TEST.Dump(mresults); )

    // Scores can get very large and even though 32 bit signed ints should be
    // enough we handle the case of a score overflow by stopping the identification
    // setting m_idTime to the maximum allowed.
    double curIdTime = m_IdTime;
    bool scoreOverflow = mresults.GetTopScore(1) >= std::numeric_limits<int>::max()-10E6;
    m_IdTime = scoreOverflow ? Pms::MaxIdTime : m_IdTime;
    
    // TODO:
    // Restructure this code. It's quite ugly.

    if(m_IdType == FUZZY_IDENTIFICATION)
    {

       if(processed && mresults.Top_K.size() >= 1)
       {
           // Get the current top match(es)
           const std::list<int> &BestQis = mresults.GetTop(1);

           // Get top 2 results
           float top1 = mresults.GetTopScore(1);
           float top2 = mresults.GetTopScore(2);

           float conf = top1 / (top1 + top2);

           // Compute mean confidence and do classification
           // of each current best match (there may be ties).
           for(int Qi : BestQis)
           {
               IdAcc_t& Ac = m_MatchAcc[Qi];
               Ac.Conf += conf;
               Ac.Time += dt_proc;
               Ac.Steps++;
               float Hu = Ac.Conf / Ac.Steps;
               float Ti = m_IdTime < Pms::MaxIdTime ? Ac.Time : m_IdTime;
               DoClassification(Hu, Ti);
           }
       }
       else
           // The classification must be performed at each step even if
           // there are no best matches as it is the classifier that will
           // stop the identification based on elapsed time. So, if no
           // candidates are found do classification based on time only.
           DoClassification(0, m_IdTime);
    }
    else if(m_IdType == BINARY_IDENTIFICATION)
    {

       if(processed && mresults.Top_K.size() >= 1)
       {
          // Get the current top match(es)
          const std::list<int> &BestQis = mresults.GetTop(1);

          // Get top 2 results
          float top1 = mresults.GetTopScore(1);
          float top2 = mresults.GetTopScore(2);

          float conf = top1 / (top1 + top2);

          // Here we use the instantaneous confidence
          for(int Qi : BestQis)
          {
              IdAcc_t& Ac = m_MatchAcc[Qi];
              Ac.Conf = conf;
              Ac.Steps = 1;
          }

		  if(conf >= m_BinaryIdThreshold && m_IdTime >= m_BinaryIdMinTime)
             FillResults( Audioneex::IDENTIFIED );
       }

       // If the max identification time has elapsed and no match has
       // been found, flush the buffers and check if there is a match
       // before returning no results.
       if(processed && m_IdTime >= Pms::MaxIdTime && m_IdMatches.empty()){
          Flush();
          if(m_IdMatches.empty()){
             IdMatch match = {};
             m_IdMatches.push_back(match);
          }
       }
    }
    else
       throw Audioneex::InvalidParameterException("Invalid classification type");

    // Restore the identification time if a score overflow occurred
    if(scoreOverflow){
       m_IdTime = curIdTime;
       WARNING_MSG("Score overflow occurred. Stopped identification.")
    }

}

// ----------------------------------------------------------------------------

int Audioneex::RecognizerImpl::DoClassification(float Hu, float dT)
{
    m_Classifier.SetMode( m_IdMode );

    int cresult = m_Classifier.Process(Hu, dT);

    // Build the results set based on classification.
    // NOTE: Some of the fuzzy classifier output values (specifically
    //       IDENTIFIED, SOUNDS_LIKE and UNIDENTIFIED) are also final
    //       states of the automaton modeling the identification
    //       process, so it should terminate at one of those states.

    // A reference fingerprint was identified as the best match.
    if(cresult == MatchFuzzyClassifier::IDENTIFIED)
    {
        FillResults( cresult );
        //DEBUG_MSG("- IDENTIFIED in "<<dT<<" s")
    }
    // Not enough evidence collected. More audio data needed...
    else if(cresult == MatchFuzzyClassifier::LISTENING){
    }
    // A reference fingerprint shows similarities with the query but
    // evidence is not strong enough to make a clear identification.
    else if(cresult == MatchFuzzyClassifier::SOUNDS_LIKE)
    {
        FillResults( cresult );
        //DEBUG_MSG("- SOUNDS LIKE ")
    }
    // No reference fingerprint shows similarities with the query.
    else if(cresult == MatchFuzzyClassifier::UNIDENTIFIED)
    {
        // Flush the buffers before returning no results.
        // Although recursive, it will only be invoked once.
        Flush();

        // If the flush didn't produce results, return empty set.
        if(m_IdMatches.empty()){
           m_IdMatches.clear();
           IdMatch match = {};
           m_IdMatches.push_back(match);
        }

        //DEBUG_MSG("- COULD NOT IDENTIFY THE AUDIO ")
    }
    else
        throw std::logic_error("Invalid classification results");

    return cresult;
}

// ----------------------------------------------------------------------------

void Audioneex::RecognizerImpl::FillResults(int cresult)
{
    const MatchResults_t &mresults = m_Matcher.GetResults();

    // Get the best match(es) (there may be ties)

    IdMatch match;

    m_IdMatches.clear();

    const std::list<int> &best_matches = mresults.GetTop(1);

    int best_score = mresults.GetTopScore(1);

    for(int Qi : best_matches)
    {
        match.FID = Qi;
        // Scores may get very big, so do some scaling
        match.Score = best_score / 1000.f;
        match.Confidence = m_MatchAcc[Qi].Conf / m_MatchAcc[Qi].Steps;
        match.IdClass = static_cast<eIdClass>( cresult );
        match.CuePoint = static_cast<uint32_t>( mresults.GetCuePoint(Qi) );

        m_IdMatches.push_back(match);
    }

    // Insert an empty element at the end (End Of List marker)
    match = IdMatch();
    m_IdMatches.push_back(match);
}

// ----------------------------------------------------------------------------

Audioneex::IdMatch* Audioneex::RecognizerImpl::GetResults()
{
    return m_IdMatches.empty() ? NULL : m_IdMatches.data();
}

// ----------------------------------------------------------------------------

void Audioneex::RecognizerImpl::Flush()
{
    float To = m_Matcher.GetMatchTime();

    // perform matching of LF stream
    int flushed = m_Matcher.Flush();

    // Process results, if any
    if(flushed)
       ProcessMatchResults( flushed, m_Matcher.GetMatchTime() - To );
}

// ----------------------------------------------------------------------------

void Audioneex::RecognizerImpl::Reset()
{
    m_IdMatches.clear();
    m_MatchAcc.clear();
    m_IdTime = 0.0;
    m_Matcher.Reset();
    m_Fingerprint.Reset();
}

// ----------------------------------------------------------------------------

void Audioneex::RecognizerImpl::SetDataStore(DataStore *dstore)
{
    m_Matcher.SetDataStore(dstore);
}

// ----------------------------------------------------------------------------

void Audioneex::RecognizerImpl::SetAudioBufferSize(float seconds)
{
    if(seconds < 1 )
       throw Audioneex::InvalidParameterException("Invalid buffer size. Must be >= 1 s");

    size_t bufferSize = Pms::Fs * seconds * Pms::Ca;
    m_AudioBuffer = AudioBlock<float>(bufferSize, Pms::Fs, Pms::Ca, 0);

}

