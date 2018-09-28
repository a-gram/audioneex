/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef MATCHER_H
#define MATCHER_H

#include <vector>
#include <map>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/container/flat_map.hpp>

#include "Fingerprint.h"
#include "Codebook.h"
#include "DataStore.h"
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

// -------- Internal exceptions introduced by the following classes -------

class InvalidMatchSequenceException : public Audioneex::Exception {
  public: explicit InvalidMatchSequenceException(const std::string& msg) :
                   Audioneex::Exception(msg) {}
};

/// Forward decls
struct Qhisto_t;
struct Ac_t;

/// Blessing Typedefs
typedef std::map<int, std::list<int>, std::greater<int> >  hashtable_Qi;
typedef boost::unordered::unordered_map<int, Ac_t>         hashtable_Qc;
typedef boost::unordered::unordered_map<int, lf_pair>      hashtable_lf_pair;
typedef boost::unordered::unordered_map<int, qlf_pair>     hashtable_qlf_pair;
//typedef std::map<int, std::list<Qhisto_t>, std::greater<int> > hashtable_Qhisto;
typedef boost::container::flat_map<int, std::list<Qhisto_t>, std::greater<int> > hashtable_Qhisto;
typedef boost::unordered::unordered_map<int, std::unique_ptr <DataStoreImpl::PListIterator> > hashtable_PLIter;
typedef boost::unordered::unordered_set<int>               hashtable_EOLIter;

/// Time histogram structure
struct AUDIONEEX_API_TEST HistoBin_t
{
    struct Info_t
    {
        int CandLF {0};
        int Pivot  {0};
    };

    typedef boost::unordered::unordered_map<int, Info_t> info_table;

    int score      {0};
    int last_T     {0};
    int torder     {0};
    bool scored    {false};
    info_table Info;

    void Reset()
	{
        score  = 0;
        last_T = 0;
        torder = 0;
        scored = false;
        Info.clear();
    }
};

/// Time histogram bin structure
struct AUDIONEEX_API_TEST Qhisto_t
{
    std::vector<HistoBin_t> Ht;
    int Bmax {0};
    int Qi   {0};

    Qhisto_t(size_t size=0) : Ht(size) {}

    void Reset()
	{
        for(HistoBin_t &bin : Ht) {
            bin.Reset();
		}
        Bmax=0; Qi=0;
    }

    void ResetBinScoredFlag()
	{
        for(HistoBin_t &bin : Ht) {
            bin.scored = false;
		}
    }

    HistoBin_t& operator[](size_t bin)
	{
        return Ht[bin];
    }

    const HistoBin_t& operator[](size_t bin) const
	{
        return Ht[bin];
    }

    void operator+=(const Qhisto_t &Qh)
	{
         assert(Qh.Ht.size() == Ht.size());
         for(size_t bin=0; bin<Ht.size(); bin++)
             Ht[bin].score += Qh.Ht[bin].score;
    }

    void Resize(size_t size) { Ht.resize(size); }

};

/// Candidate structure
struct AUDIONEEX_API_TEST Ac_t
{
    int Ac     {0};
    int Tmatch {0};
};

/// Match results structure
struct AUDIONEEX_API_TEST MatchResults_t
{
    hashtable_Qc   Qc;      // list of candidate Qi accumulators
    hashtable_Qi   Top_K;   // list of top-k Qi matches <score, tie list>

    /// Get the k-th elements (tie list)
    std::list<int> GetTop(int k) const 
	{
        if(Top_K.empty() || k>Top_K.size())
           return std::list<int>();
        else{
           hashtable_Qi::const_iterator it = Top_K.cbegin();
           for(int i=1; i<k; ++i, ++it);
           return it->second;
        }
    }

    /// Get the k-th score
    int GetTopScore(int k) const 
	{
        if(Top_K.empty() || k>Top_K.size())
           return 0;
        else{
           hashtable_Qi::const_iterator it = Top_K.cbegin();
           for(int i=1; i<k; ++i, ++it);
           return it->first;
        }
    }

};


// ----------------------------------------------------------------------------


class AUDIONEEX_API_TEST Matcher
{
    std::unique_ptr <Codebook>       m_AudioCodes;

    MatchResults_t                   m_Results;
    std::vector<QLocalFingerprint_t> Xk;
    std::vector<uint32_t>            m_XkSeq;

    hashtable_Qhisto                 m_TopKMc;
    Qhisto_t                         m_H;
    
	Audioneex::DataStore*            m_DataStore {nullptr};

    /// Pointer to the start of current LF batch being matched.
    int m_ko     {0};

    /// The duration of the audio being matched so far from last resetting.
    int m_ko_T   {0};

    /// Number of processing steps performed so far.
    int m_Nsteps {0};

    /// Minimum score to be considered in the match stage.
	/// Anything smaller will be ignored.
    const static int MIN_ACCEPT_SCORE = Pms::Smax * 2;

#ifdef PLOTTING_ENABLED
    std::vector< std::vector<float> > Mc;
#endif

    bool  ValidQuerySequence();
    void  DoMatch(int ko, int kn);
    void  FindCandidatesBWords(int ko, int kn);
    void  FindCandidatesSWords(int ko, int kn);

friend class RecognizerImpl;

 public:

    Matcher();
	~Matcher() = default;

    /// Process a stream of LF. This method requires the client to perform the
    /// fingerprinting of the audio.
    /// Return the number of processed LFs (the matcher might not process the
    /// given LFs stream but just buffer it until the minimum required amount
    /// to execute a processing step is available).
    int Process(const lf_vector &lfs);

    /// Get the current match results. This method can be called at any point
    /// during the matching stage (after Process()) to analyze the matching
    /// status. Note that the matcher does not perform the final identification,
    /// intended as the final classification of whether there is a best match
    /// or not, but merely provides the status of the best match search at each
    /// processing step. The task of classification is left to an external component
    /// (the Recognizer) which will decide by analysing the match results.
    const MatchResults_t&  GetResults() const { return m_Results; }

    /// Flush any remaining LFs in the query sequence.
    /// Return the number of flushed LFs, if any.
    int Flush();

    /// Reset the internal state of the matching. This method should be called
    /// by the classification module once a classification has been made, or if
    /// the classification cannot be made within a set period of time.
    /// A call to this method is not necessary if the Matcher instance is not
    /// being reused after the classification.
    void Reset();

    /// Get the length of the audio being matched since last resetting (or start)
    /// NOTE: This is an approximation of the duration since it is computed from
    /// the T value of the last LF received, which does not exactly fall at the
    /// end of the audio snippet, so one must not rely on this value if an exact
    /// time duration is needed.
    float GetMatchTime() const { return m_ko_T * Pms::dt; }

    /// Get the number of processing steps performed so far. Note that the number
    /// of processing steps is not necessrily equal to the number of times the
    /// Process() method is called as the matcher may buffer the data and defer
    /// the processing if it does not receive enough data.
    float GetStepsCount() const { return m_Nsteps; }

    /// Set the data provider
    void SetDataStore(Audioneex::DataStore* dstore);

    /// Getter
    DataStore* GetDataStore() const { return m_DataStore; }

    /// Set the maximum duration of the recordings in the database.
    /// This value will be used internally to optimize the efficiency of some data
    /// structures used during the matching process. It is not mandatory to set it
    /// but doing so excessive reallocations of such structures might be avoided.
    void SetMaxRecordingDuration(size_t duration);

};

}// end namespace Audioneex

#endif // MATCHER_H
