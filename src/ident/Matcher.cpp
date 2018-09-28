/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#include "common.h"
#include "Parameters.h"
#include "Matcher.h"
#include "Indexer.h"
#include "AudioCodes.h"
#include "Utils.h"

#ifdef TESTING
 #include "Tester.h"
#endif

TEST_HERE( namespace { Audioneex::Tester TEST; } )


Audioneex::Matcher::Matcher()
{
    // Set the time histogram size to a value that is adequate to accomodate
    // recordings up to 15 mins long (should be adequate for most recordings,
    // in case it is not, the histogram will be reallocated).
    // NOTE: This value is arbitrary and can be changed by clients to a proper
    //       value by using SetMaxRecordingDuration().
    size_t H_size =  900 / (Pms::dt * Pms::Tk);

    m_H.Resize(H_size);

    Xk.reserve(256);
    m_XkSeq.reserve(256);
}

// ----------------------------------------------------------------------------

int Audioneex::Matcher::Process(const lf_vector &lfs)
{
    // Check whether we have valid data store
    if(m_DataStore == nullptr)
       throw Audioneex::InvalidParameterException
             ("No data provider set.");

    // Nothing to process
    if(lfs.empty()) return 0;

    // Append LF stream to query sequence
    for(const auto &lf : lfs)
    {
        Codebook::QResults quant = m_AudioCodes->quantize(lf);
        QLocalFingerprint_t QLF;
        QLF.T = lf.T;
        QLF.F = lf.F;
        QLF.W = quant.word;
        QLF.E = quant.dist; // Clipped. See NOTE in Codebook::quantize()
        Xk.push_back(QLF);
        m_XkSeq.push_back(lf.ID);
    }

    // Validate query sequence
    if(!ValidQuerySequence())
        throw Audioneex::InvalidMatchSequenceException
              ("Invalid query sequence. LF id's must be sequential.");

    int processed = 0;

    // Process batches of Nk LF.
    while(Xk.size() - m_ko >= Pms::Nk)
    {
        int Xk_T = Xk[m_ko + Pms::Nk - 1].T;

        TEST_HERE( DEBUG_MSG( "INPUT: Processing "<<Pms::Nk<<" LFs ("<<(Xk_T - m_ko_T)*Pms::dt<<" s)" ) )

        DoMatch(m_ko, m_ko+Pms::Nk);

        // Advance to next batch
        m_ko += Pms::Nk;
        m_ko_T = Xk_T;

        m_Nsteps++;
        processed += Pms::Nk;
    }

    return processed;
}

// ----------------------------------------------------------------------------

int Audioneex::Matcher::Flush()
{
    // Check whether we have a valid data store
    if(m_DataStore == nullptr)
       throw Audioneex::InvalidParameterException
             ("No data provider set.");

    // Validate query sequence
    if(!ValidQuerySequence())
        return 0;

    // Compute number of LFs to flush
    int Nlf = Xk.size() - m_ko;

    // Sequence too short. Return.
    if(Nlf<2){
       TEST_HERE( DEBUG_MSG( "INPUT: Flushing aborted ("<<Nlf<<" LF)" ) )
       return 0;
    }

    int Xk_T = Xk[m_ko + Nlf - 1].T;
	
    TEST_HERE( DEBUG_MSG( "INPUT: Flushing "<<Nlf<<" LFs ("<<(Xk_T - m_ko_T)*Pms::dt<<" s)" ) )
	
    DoMatch(m_ko, m_ko+Nlf);
    m_ko += Nlf;
    m_ko_T = Xk_T;
    m_Nsteps++;
	
    return Nlf;
}

// ----------------------------------------------------------------------------

bool Audioneex::Matcher::ValidQuerySequence()
{
    // The LFs in the query sequence must have sequential IDs starting from 0
    for(size_t k=0; k<m_XkSeq.size(); k++)
        if(m_XkSeq[k] != k)
           return false;

    return true;
}

// ----------------------------------------------------------------------------

void Audioneex::Matcher::Reset()
{
    Xk.clear();
    m_XkSeq.clear();
    m_TopKMc.clear();

    m_Results = MatchResults_t();
    m_ko      = 0;
    m_ko_T    = 0;
    m_Nsteps  = 0;
}

// ----------------------------------------------------------------------------

void Audioneex::Matcher::SetMaxRecordingDuration(size_t duration)
{
    size_t H_size =  duration / (Pms::dt * Pms::Tk);
    m_H.Resize(H_size);
}

// ----------------------------------------------------------------------------

void Audioneex::Matcher::SetDataStore(DataStore* dstore)
{
    m_DataStore = dstore;

    // Check whether we have valid data store
    if(m_DataStore == nullptr)
       throw Audioneex::InvalidParameterException
             ("Invalid data store set (null).");

    // Get codebook (audio codes)
    if(!m_AudioCodes)
    {
       m_AudioCodes = Codebook::deserialize(GetAudioCodes(), GetAudioCodesSize());

       if(!m_AudioCodes)
          throw Audioneex::InvalidAudioCodesException
                ("Could't get audio codes.");
    }
}


// ----------------------------------------------------------------------------


void Audioneex::Matcher::DoMatch(int ko, int kn)
{
    // Search the Database for candidate Qi's similar to the current query LFs.

    FindCandidatesSWords(ko,kn);

    TEST_HERE( TEST.Dump(m_TopKMc); )

    if(!m_TopKMc.empty())
    {

        for(auto &e : m_TopKMc) 
        {
            std::list<Qhisto_t> &tlist = e.second;
			
            for(Qhisto_t &H : tlist) 
            {
                 Ac_t& Accum = m_Results.Qc[H.Qi];
                 Accum.Ac += H.Ht[H.Bmax].score;
                 Accum.Tmatch = (Pms::Tk * H.Bmax + Pms::Tk / 2) * Pms::dt;
            }
        }
		
        // Update the final top-k list

        m_Results.Top_K.clear();

        for(auto &e : m_Results.Qc)
	{
            int Qi = e.first;
            int score = e.second.Ac;
            m_Results.Top_K[score].push_back(Qi);
			
            if(m_Results.Top_K.size() > Pms::TopK)
               m_Results.Top_K.erase(--(m_Results.Top_K.end()) );
        }

        m_TopKMc.clear();
    }
}

// ----------------------------------------------------------------------------

void Audioneex::Matcher::FindCandidatesSWords(int ko, int kn)
{
	hashtable_PLIter  iterators;
    hashtable_EOLIter EOL_iterators;

    uint32_t FIDcurr = 1;

    int NLFs  = kn - ko;

    // Score fingerprints in DaaT fashion until all postings
    // list iterators reach EOL.
    do
    {
        for(size_t k=ko; k<kn; k++)
        {
            // Create term <word|channel>
            int chan = (Xk[k].F - Pms::Kmin + 1) / Pms::qF;
            int term = (Xk[k].W << 6) | chan;

            // Get postings list iterator for term from cache.
            // If we get a miss then get a new one;
            auto &it = iterators[term];

            if(it.get() == nullptr)
               it.reset(DataStoreImpl::GetPListIterator(m_DataStore, term));

            DataStoreImpl::Posting_t& post = it->get();

            assert(post.empty() ? 1 : post.FID > 0);

            // If the iterator is at EOL add it to 'exhausted' iterators
            // list. (NOTE: An EOL iterator must be added only once).
            if(post.empty())
               EOL_iterators.insert(term);

            if(post.FID == FIDcurr)
            {
               for(size_t m=0; m<post.tf; m++)
               {
                   // -------- Time clustering ----------

                   int Sij = post.LID[m];
                   int Sij_t = post.T[m];
                   int Sij_e = post.E[m];

                   int bin = Sij_t / Pms::Tk;

                   // Check that time values are within the histo.
                   // Resize if necessary.
                   if(bin >= m_H.Ht.size()){
                      m_H.Resize(bin*1.1);
                      WARNING_MSG("Matcher: Ht reallocation occurred.")
                   }

                   // Check whether a candidate has already been scored for this bin
                   // (a query LF should score only 1 candidate per bin)
                   if(!m_H[bin].scored)
                   {
                       // Check whether the current candidate LF has already been scored
                       // in this bin and skip scoring if true
                       bool CanScore = false;

                       HistoBin_t::Info_t &info = m_H[bin].Info[Sij];

                       if(info.CandLF==0 && info.Pivot==0){
                           info.CandLF = k;
                           info.Pivot = 1; // NOTE: Pivot id must be 1-based
                           CanScore = true;
                       }

                       if(CanScore)
                       {
                           int tdiff = Sij_t - m_H[bin].last_T;
                           if(abs(tdiff)<=2) tdiff=0;

                           float Wtp = 1.0f - static_cast<float>(abs(Xk[k].E - Sij_e)) /
                                              static_cast<float>(Pms::IDI);

                           // Time Proximity score (weighed by similarity value)
                           int score_tp = 1000 * Wtp;

                           if(tdiff>=0)
                               m_H[bin].torder++;

                           float Wto = static_cast<float>(m_H[bin].torder) /
                                       static_cast<float>(m_H[bin].Info.size());

                           // Time order score
                           int score_to = (tdiff >= 0) ? 1000 * Wto : 0;

                           m_H[bin].score += (score_tp + score_to);
                           m_H[bin].last_T = Sij_t;
                           //H[bin].Info[Sij].LF = Xk[k].ID;

                           // Update max bin index
                           if(m_H[bin].score > m_H[m_H.Bmax].score)
                               m_H.Bmax = bin;

                           // Mark the bin as 'scored' to avoid multiple scoring.
                           m_H[bin].scored = true;
                       }
                   }

                   // -------- End Time clustering ----------

               }// end for(m)

               m_H.ResetBinScoredFlag();
               it->next();
            }

        }// end for(k)

        // Process histogram for current fingerprint

        m_H.Qi = FIDcurr;

        // Get histo max score and store in top-k list

        int maxScore = m_H[m_H.Bmax].score;

        // Skip scores that are too low
        if(maxScore > MIN_ACCEPT_SCORE)
        {
           // If the tie lists get too big it may impact performances
           // so as a provisional solution we simply truncate them at
           // a predefined length. A better solution would be to find
           // a method that strongly disambiguates the candidates.

           if(m_TopKMc[maxScore].size() <= 10)
              m_TopKMc[maxScore].push_back(m_H);

           if(m_TopKMc.size() > Pms::TopK)
              m_TopKMc.erase(--(m_TopKMc.end()) );
        }

        m_H.Reset();

        FIDcurr++;
    }
    while(EOL_iterators.size() < iterators.size());
}

