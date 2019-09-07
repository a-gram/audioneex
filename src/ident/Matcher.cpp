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
                ("Couldn't get audio codes.");
    }
}


// ----------------------------------------------------------------------------


void Audioneex::Matcher::DoMatch(int ko, int kn)
{
    // Search the Database for candidate Qi's similar to the current query LFs.

    if(m_MatchType == MSCALE_MATCH)
       FindCandidatesSWords(ko,kn);
    else if(m_MatchType == XSCALE_MATCH)
       FindCandidatesBWords(ko,kn);
    else
       throw Audioneex::InvalidParameterException
             ("Invalid matching algorithm");
    
    TEST_HERE( TEST.Dump(m_TopKMc); )

    // Here we would evaluate the confidence and decide whether to perform
    // the reranking based on the confidence score

    // The adaptive algorithm used is a simple threshold-based
    // classification around a confidence value computed from
    // the current top-k list: if there is a clear evidence of
    // a match (confidence >= threshold) the decision is not
    // to perform reranking, otherwise reranking is applied.

    if(!m_TopKMc.empty())
    {
        // Get top 2 results
        hashtable_Qhisto::iterator it = m_TopKMc.begin();

        float top1 = it->first;
        float top2 = m_TopKMc.size() > 1 ? (++it)->first : 0;
        float conf = (2.f * top1) / (top1 + top2) - 1.f;

        assert(0<=conf && conf<=1);

        if(conf <= m_RerankThreshold)
        {
           TEST_HERE( DEBUG_MSG("Performing reranking: conf="<<conf) )
           Reranking();
           m_Results.Reranked = true;
        }
        else
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
           m_Results.Reranked = false;
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

void Audioneex::Matcher::Reranking()
{
    Qhisto_t Hr (m_H.Ht.size());

    for(hashtable_Qhisto::value_type &e : m_TopKMc)
    {
        // get list of Qi's with this score (tie list)
        std::list<Qhisto_t> &tlist = e.second;

        // perform t-f coherence matching on Qi's top bins
        for(Qhisto_t &H : tlist)
        {
         // ============ Find top-n bins in Qi's histo ============

         // The t-f matching is performed on the top-n bins in each of the
         // top-k Time Histograms built in Time Clustering phase rather than
         // on the top bin only. This will increase the discrimination power.

         // Choosing a high value of n will test more bins in H, increasing the
         // probability of correct match, but will also affect performances as
         // it will require more access to the data store, so the right choice
         // of n is a tradeoff between accuracy and speed.

         // The approach used here is to select the top bins evenly distributed
         // across the histogram by scanning it with a fixed size window. The
         // number of bins selected using this method depends on the lenght of the
         // recording and the time complexity is approx O(Nh/r) where Nh is the
         // size of the histogram and r the window's radius.
         // Alternatively we can select the top n bins regrdless of their distribution
         // in the histogram, which will have a constant time complexity O(k=n).

            int TopBin = 0,
                TopBinScore = 0,
                Ht_lbin = H.Ht.size() - 1;

            // Only process the used histogram bins
            for(; Ht_lbin>=0 && H.Ht[Ht_lbin].score==0; Ht_lbin--);

            for(size_t i=0; i<=Ht_lbin; i++)
            {
                int lb = i - 3;  //<-- interval radius
                int rb = i + 3;

                if(lb<0) lb=0;
                if(rb>Ht_lbin) rb=Ht_lbin;

                bool ismax = true;
                for(int j=lb; lb<rb && j<=rb; j++)
                    if(H.Ht[j].score > H.Ht[i].score)
                        ismax=false;

                // If peak bin found
                if(ismax && H.Ht[i].score > Matcher::MIN_ACCEPT_SCORE*1.5)
                {
                    // Perform T-F coherence on bin
                    GraphMatching(H, i, Hr);

                    // Update Qi score in candidates set (or insert it if doesn't exist)

					if(Hr.Ht[Hr.Bmax].score > 0)
                       m_Results.Qc[H.Qi].Ac += Hr.Ht[Hr.Bmax].score;

                    if(Hr.Ht[Hr.Bmax].score > TopBinScore){
                        TopBinScore = Hr.Ht[Hr.Bmax].score;
                        TopBin = Hr.Bmax;
                    }
                    Hr.Reset();
                }
            }// end for(i)

            // Give an estimate of the match time point within the recording by
            // a fixed linear interpolation at the bin centre. The time point is
            // supposed to be within the time bin with the max score.
			if(TopBinScore > 0)
               m_Results.Qc[H.Qi].Tmatch = (Pms::Tk * TopBin + Pms::Tk / 2) * Pms::dt;

        }// end foreach(H)
    }
}

// ----------------------------------------------------------------------------

void Audioneex::Matcher::FindCandidatesBWords(int ko, int kn)
{
	hashtable_PLIter  iterators;
    hashtable_EOLIter EOL_iterators;

    int NLFs  = kn - ko;

    uint32_t FIDcurr = 1;

    // We cannot process streams shorter than 2 LFs
    if(NLFs < 2) return;

    // Score fingerprints in DaaT fashion until all postings
    // list iterators reach EOL.
    do
    {
        for(size_t k=ko; k<kn; k++)
        {
            int Wpivot = Xk[k].W;
            int Bpivot = Xk[k].F / IndexerImpl::qB;
            
            // Compute bi-terms
            // TODO: We need to compute the terms only once, so this code
            //       should be put outside the main loop.
            for(size_t j=k+1, dN=0; dN<IndexerImpl::Dmax && j<Xk.size()/*kn*/; j++)
            {
                int dt = Xk[j].T - Xk[k].T;
                assert(dt>=0);
                if(dt > IndexerImpl::Tmax)
                    break;

                int Bpair = Xk[j].F / IndexerImpl::qB;

                // If the LF is in the same band as pivot's do pairing
                if(Bpair == Bpivot)
                {
                   int W2  = Xk[j].W;
                   int Vpt = Xk[j].T / Pms::qT - Xk[k].T / Pms::qT;
                   int Vpf = Xk[j].F / Pms::qF - Xk[k].F / Pms::qF;

                   assert(0 <= Wpivot && Wpivot <= Pms::Kmed);
                   assert(0 <= W2 && W2 <= Pms::Kmed);
                   assert(0 <= Vpt && Vpt <= IndexerImpl::Vpt_max);
                   assert(0 <= abs(Vpf) && abs(Vpf) <=  IndexerImpl::Vpf_max);

                   int term = Wpivot << IndexerImpl::W1_SHIFT |
                              Bpivot << IndexerImpl::B_SHIFT |
                              W2 << IndexerImpl::W2_SHIFT |
                              Vpt << IndexerImpl::VPT_SHIFT |
                              (Vpf & 0x3F);

                   // Get postings list iterator for term from cache.
                   // If we get a miss then get a new one;
                   std::unique_ptr <DataStoreImpl::PListIterator> &it = iterators[term];

                   if(!it)
                      it.reset(DataStoreImpl::GetPListIterator(m_DataStore, term));

                   DataStoreImpl::Posting_t& post = it->get();

                   assert(post.empty() ? 1 : post.FID > 0);

                   // If the iterator is at EOL add it to 'exhausted' iterators
                   // list. (NOTE: An EOL iterators must be added only once).
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
                              bool CanScore;

                              HistoBin_t::Info_t &info = m_H[bin].Info[Sij];

                              if(info.CandLF==0 && info.Pivot==0){
                                  info.CandLF = k;
                                  info.Pivot = k+1; // NOTE: Pivot id must be 1-based
                                  CanScore = true;
                              }else
                                  CanScore = info.Pivot-1 == k;

                              if(CanScore)
                              {
                                  // TODO: Use quantized pivots' times to mitigate inaccuracies
                                  //       in the time order?

                                  int tdiff = Sij_t - m_H[bin].last_T;
                                  if(abs(tdiff)<=2) tdiff=0;

                                  // Time Proximity score (weighed by similarity value)
                                  float Wtp = 1.0f - static_cast<float>(abs(Xk[k].E - Sij_e)) /
                                                     static_cast<float>(Pms::IDI);

                                  int score_tp = Pms::Smax * Wtp;

                                  // TODO: Can we do Time Order using the LFs ID ?
                                  //       This will avoid using the time value which must be
                                  //       fetched from the database or included in the postings.
                                  if(tdiff>=0)
                                      m_H[bin].torder++;

                                  float Wto = static_cast<float>(m_H[bin].torder) /
                                              static_cast<float>(m_H[bin].Info.size());

                                  int score_to = (tdiff >= 0) ? Pms::Smax * Wto : 0;

                                  m_H[bin].score += (score_tp + score_to);
                                  m_H[bin].last_T = Sij_t;

                                  // Update max bin index
                                  if(m_H[bin].score > m_H[m_H.Bmax].score)
                                      m_H.Bmax = bin;

                                  // Mark the bin as 'scored' to avoid multiple scoring.
                                  m_H[bin].scored = true;
                              }
                          }

                          // -------- End Time clustering ----------
                      }

                      m_H.ResetBinScoredFlag();
                      it->next();
                   }
                   dN++;
                }
            }
        }// end for(k)

        // Process histogram for current fingerprint

        m_H.Qi = FIDcurr;

        // Get histo max score and store in top-k list

        int maxScore = m_H[m_H.Bmax].score;

        // Skip scores that are too small
        if(maxScore > MIN_ACCEPT_SCORE)
        {
           // If the tie lists get too big it may impact performances
           // so as a provisional solution we simply truncate them at
           // a predefined length. A better solution would be to find
           // a method that strongly disambiguates the candidates.

           if(m_TopKMc[maxScore].size() < 10)
              m_TopKMc[maxScore].push_back(m_H);

           if(m_TopKMc.size() > Pms::TopK)
              m_TopKMc.erase(--(m_TopKMc.end()) );
        }

        m_H.Reset();
        FIDcurr++;
    }
    while(EOL_iterators.size() < iterators.size());
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

// ----------------------------------------------------------------------------

void Audioneex::Matcher::GraphMatching(Qhisto_t &Qhisto_i, int bin, Qhisto_t &Qhisto)
{
    hashtable_qlf_pair  Hx,Hq;

    int Qi = Qhisto_i.Qi;

    // Score all LF pairs <k,Sij> in the specified histo bin
    for(HistoBin_t::info_table::value_type &elem : Qhisto_i[bin].Info)
    {
        // Get the <query LF, candidate LF> pair.
        // The pair <k,Sij> is stored in the bin's info table where
        // the key is Sij and k is the QueryLF field in the info struct.
        HistoBin_t::Info_t &Binfo = elem.second;

        int k = Binfo.CandLF;
        int Sij = elem.first;

        assert(0<=k && k<Xk.size());

int common_edges=0;

        // For each candidate we perform the t-f matching on a neighborhood
        // of Ntf LFs in the query and candidate sequences. Here the
        // starting and ending points of this neighborhood are computed
        // for the query sequence X(k).
        int ks = (k>=Pms::Ntf/2) ? k-Pms::Ntf/2 : 0;
        int ke = (Xk.size()-k-1 >= Pms::Ntf/2) ? k+Pms::Ntf/2 : Xk.size()-1;
        int Nx = ke - ks + 1;

        assert(0<=ks && ks<Xk.size());
        assert(0<=ke && ke<Xk.size());
        assert(ks <= ke);

        // Build graph for Xh
        BuildGraphs(&Xk[ks], Nx, k-ks, Hx);

        // IMPORTANT:
        // Here we're getting input from external clients that could potentially
        // crash the whole application if invalid, so runtime checks are needed.

        // We need to know the size of the fingerprint in order to get
        // the correct subsequence Qh
        size_t fp_size = m_DataStore->GetFingerprintSize(Qi);

        if(fp_size == 0)
           throw Audioneex::InvalidFingerprintException
                ("Zero sized fingerprint received. Maybe not existent? "
                 "Please check the fingerprint database (FID="+Utils::ToString(Qi)+")");

        if(fp_size % sizeof(QLocalFingerprint_t) != 0)
           throw Audioneex::InvalidFingerprintException
                ("Invalid fingerprint data. The fingerprint may be corrupt."
                 "Please check the fingerprint database (FID="+Utils::ToString(Qi)+")");

        int Qlen = fp_size / sizeof(QLocalFingerprint_t);

        // Starting and ending points for neighborhood of Qij (in LIDs)
        int ss = Sij - std::min<int>(Sij, k-ks);
        int se = Sij + std::min<int>(ke-k, Qlen-Sij-1);
        int Nh = se - ss + 1;

        // NOTE:
        // These checks can fail as a result of the clients using inconsistent
        // datastores. For example, mixing a set of Qfingerprints with an index
        // generated by a different set of Qfingrprints may trigger these asserts.

        if(Sij<0 || Sij>=Qlen)
           throw Audioneex::InvalidIndexDataException
                ("Invalid LID. The index appears to be inconsistent.");

        assert(0<=ss && ss<Qlen);
        assert(0<=se && se<Qlen);
        assert(ss <= se);

        // Get the fingerprint chunk corresponding to the subsequence Qh

        size_t bstart = ss * sizeof(QLocalFingerprint_t);
        size_t Qhsize = Nh * sizeof(QLocalFingerprint_t);

        size_t rsize;
        const uint8_t* pQh = m_DataStore->GetFingerprint(Qi, rsize, Qhsize, bstart);

        if(rsize == 0 || pQh == nullptr)
           throw Audioneex::InvalidFingerprintException
                ("No fingerprint data received. Maybe not existent? "
                 "Please check the fingerprint database (FID="+Utils::ToString(Qi)+")");

        if(rsize != Qhsize)
           throw Audioneex::InvalidFingerprintException
                ("Invalid fingerprint data size. Should be "+Utils::ToString(Qhsize)+". "
                 "Please check the fingerprint database (FID="+Utils::ToString(Qi)+")");

        const QLocalFingerprint_t* Qh = reinterpret_cast<const QLocalFingerprint_t*>(pQh);

        // Build graph for Qh
        BuildGraphs(Qh, Nh, Sij-ss, Hq);

        int score = 0;

        // Perform graph matching
        for(hashtable_qlf_pair::value_type elem : Hq)
        {
            // Get edge (it's the key to the table)
            int e = elem.first;

            // Check for common edge in query graph
            if(Hx.find(e) != Hx.end())
            {
                // Get endpoint LFs for matching edge in G(X,E) and G(Q,E)
                qlf_pair &Pq = Hq[e];
                qlf_pair &Px = Hx[e];

                float sim1 = Pq.first->W == Px.first->W ? 1000.f : 0.f;
                float sim2 = Pq.second->W == Px.second->W ? 1000.f : 0.f;

                // Compute similarity weights.
                float Wsim1 = 1.0f - static_cast<float>(abs(Pq.first->E - Px.first->E)) /
                                     static_cast<float>(Pms::IDI);

                float Wsim2 = 1.0f - static_cast<float>(abs(Pq.second->E - Px.second->E)) /
                                     static_cast<float>(Pms::IDI);

                // Increase score by edge match score
                score += 1000;
common_edges++;

                // Increase score by LFs match score
                score += sim1 * Wsim1;
                score += sim2 * Wsim2;

                 // Do time binning of score for current common edge
                int Hbin1 = Pq.first->T / Pms::Tk;
                int Hbin2 = Pq.second->T / Pms::Tk;

                // Check that time values are within the histo.
                // Resize if necessary.
                if(Hbin1 >= m_H.Ht.size() || Hbin2 >= m_H.Ht.size()){
                   Qhisto.Resize( std::max<int>(Hbin1,Hbin2) * 1.1 );
                   WARNING_MSG("Matcher: Ht reallocation occurred.")
                }

                // If both matching LFs fall in the same time bin then give the
                // bin full score. If they fall into different (adjacent) bins
                // then share the score between the 2 bins.
                Qhisto.Ht[Hbin1].score += score/2;
                Qhisto.Ht[Hbin2].score += score/2;

                // Update max bin index
                if(Qhisto.Ht[Hbin1].score > Qhisto.Ht[Qhisto.Bmax].score)
                   Qhisto.Bmax = Hbin1;
                if(Qhisto.Ht[Hbin2].score > Qhisto.Ht[Qhisto.Bmax].score)
                   Qhisto.Bmax = Hbin2;

                score = 0;
            }
        }

        Hx.clear();
        Hq.clear();
    }
}


// ----------------------------------------------------------------------------

void Audioneex::Matcher::BuildGraphs(const QLocalFingerprint_t *lfs, size_t Nlfs, int iRef, hashtable_qlf_pair &H)
{
    // Build LF sequence graph by Pair-wise Geodesic Hashing

    bool hash_ok = true;

    assert(0<=iRef && iRef<Nlfs);

    int LFref_qt = lfs[iRef].T / Pms::qT + 0.5f;
    int LFref_qf = lfs[iRef].F / Pms::qF + 0.5f;

    for(size_t i=0; i < Nlfs-1; i++)
    {
        // Do time-frequency quantization
        int LFi_qt = lfs[i].T / Pms::qT + 0.5f;
        int LFi_qf = lfs[i].F / Pms::qF + 0.5f;

        int Tt_iref = LFref_qt - LFi_qt;
        int Tf_iref = LFref_qf - LFi_qf;

        hash_ok &= (-127<=Tt_iref && Tt_iref<=128);
        hash_ok &= (-127<=Tf_iref && Tf_iref<=128);

        for(size_t j=i+1; j < Nlfs; j++)
        {
            // Do time-frequency quantization
            int LFj_qt = lfs[j].T / Pms::qT + 0.5f;
            int LFj_qf = lfs[j].F / Pms::qF + 0.5f;

            int Tt_ij = LFj_qt - LFi_qt;
            int Tf_ij = LFj_qf - LFi_qf;

            hash_ok &= (-127<=Tt_ij && Tt_ij<=128);
            hash_ok &= (-127<=Tf_ij && Tf_ij<=128);

            // Do hashing of M(i,j) entries
            int e = ((Tt_ij&0x000000FF)<<24)|
                    ((Tf_ij&0x000000FF)<<16)|
                    ((Tt_iref&0x000000FF)<<8)|
                    (Tf_iref&0x000000FF);

            // Store the edge pair in the hashtable
            H[e].first = const_cast<QLocalFingerprint_t*>(&lfs[i]);
            H[e].second = const_cast<QLocalFingerprint_t*>(&lfs[j]);
        }
    }

    // The following issues should never arise, but in the very unlikely case that
    // they do the worst that can happen is that the identification will fail or produce
    // incorrect results but the process won't crash, so we just report warnings,
    // which can be useful to track their coccurrences.

    // NOTE: This assertion may fail if the two LF in a pair (graph edge)
    //       have a time/frequency distance that overflows the hash component
    //       (8 bit signed), that is 128 quantized time units/frequency bins.
    //       With the current settings the frequency distances are always within
    //       the 8 bit component since the frequency range of interest is fixed,
    //       while the max quantized time distance of 128 units corresponds to about
    //       8,8 seconds. Since we're constructing the matching graphs using 20-30
    //       LFs (Nk parameter) which are generated (on average) by 1 second of audio
    //       it should never overflow, unless the 20-30 LFs are spread over more than
    //       8,8 seconds of audio, which is very unlikely, or there are intervals of
    //       total silence in the recording (audio samples with zero amplitude)
    //       lasting more than 8,8 seconds, which is still quite unlikely.
#ifdef TESTING
    if(!hash_ok)
       WARNING_MSG("There was some problem in the construction of the matching graphs.")
#endif
    // Note: This assertion may fail if there are LF collisions caused by the
    //       quantization process (i.e. 2 LFs are quantized to the same t-f bin,
    //       leading to multiple entries hashed to the same key in the hashtable).
    //       However, this should seldom happen if the POIs are chosen so that
    //       there cannot be 2 within the same t-f bin, which is achieved by properly
    //       setting the max suppression window.
#ifdef TESTING
    //assert(H.size() == (Nlfs*(Nlfs-1))/2);
    if(H.size() != (Nlfs*(Nlfs-1))/2)
       WARNING_MSG("LF collision detected in the graphs hash tables.")
//    DEBUG_MSG( H.size() << "/"<<int((Nlfs*(Nlfs-1))/2)<<" elements in Hx - Mat. size ="<< Nlfs<<"x"<<Nlfs )
#endif
}

