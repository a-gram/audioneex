/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifdef TESTING
 #include "Tester.h"
#endif

#include <list>
#include <vector>

#include "common.h"
#include "BlockCodec.h"
#include "Parameters.h"


namespace Audioneex
{

int BlockEncoder::Encode(const uint32_t* const* plist_chunk,
                          size_t plist_chunk_size,
                          uint8_t* enc_chunk,
                          size_t enc_chunk_size,
                          size_t &enc_bytes,
                          uint32_t FIDo,
                          bool delta_encode)
{
    // Serialize the postings list chunk into an array of integers converting
    // from the cache layout to the final layout.

    // TODO: Here we compute the exact value of the serialized chunk
    //       by scanning the whole postings list chunk. This value
    //       may be precomputed and passed as a parameter ...

    size_t m_ser_chunk_size = 0;

    for(size_t pp=0; pp<plist_chunk_size; pp++)
	{
        const uint32_t* p = plist_chunk[pp];
        m_ser_chunk_size += 2/*FID,tf*/ + *(p+1) * 3/*LID,T,E*/;
    }

    // Reallocate the buffer if data does not fit
    if(m_ser_chunk.capacity() < m_ser_chunk_size)
       m_ser_chunk.reserve(m_ser_chunk_size);

    Serialize(plist_chunk, plist_chunk_size,
              m_ser_chunk.data(), m_ser_chunk_size,
              FIDo, delta_encode);

    // IMPORTANT NOTE:
    // If not enough memory was allocated for the encoded chunk in the indexer
    // the compression will miserably fail (and the application will crash!)

    enc_bytes = m_Codec.encode(m_ser_chunk.data(), m_ser_chunk_size,
                               enc_chunk, enc_chunk_size);
    return 0;
}

// ----------------------------------------------------------------------------

int BlockEncoder::Decode(const uint8_t* enc_chunk,
                          size_t enc_chunk_size,
                          uint32_t* dec_chunk,
                          size_t dec_chunk_size,
                          size_t &dec_elem,
                          uint32_t base_FID,
                          bool delta_decode)
{
    assert(enc_chunk && dec_chunk);

    // IMPORTANT NOTE:
    // If not enough memory was allocated for the decoded chunk vector
    // the decompression will miserably fail!

    dec_elem = m_Codec.decode(enc_chunk, enc_chunk_size,
                              dec_chunk, dec_chunk_size);

    // Delta-decode the decompressed chunk
    if(delta_decode)
       if(!DeltaCodec<BlockEncoder::DDECODE>(dec_chunk, dec_elem, base_FID))
           return -1;

    return 0;
}

// ----------------------------------------------------------------------------

void BlockEncoder::Serialize(const uint32_t * const *plist_chunk,
                             size_t plist_chunk_size,
                             uint32_t *ser_chunk,
                             size_t ser_chunk_size,
                             uint32_t prev_FID,
                             bool delta_encode)
{
    size_t vpos = 0, poff0 = 0, poff1 = 0;

    for(size_t n=0; n<plist_chunk_size; n++)
    {
        const uint32_t* p = plist_chunk[n];

        // Get the FID
        uint32_t FID = *p;

        // FIDs must be strict increasing
        assert(FID > prev_FID);

        // Get the term frequency
        uint32_t tf = *(p+1);

        // Term frequency must be strict positive
        assert(tf > 0);

        // Delta-encode FID
        ser_chunk[vpos++] = FID - prev_FID;
        ser_chunk[vpos++] = tf;

        // Serialize and delta-encode the payload <{LID},{T},{E}>
        for(size_t i=0; i<tf; i++, vpos++)
		{
            poff1 = i * 3;
			
            if(i==0 || !delta_encode){
               ser_chunk[vpos] = *(p+2+poff1);       // LID
               ser_chunk[vpos+1*tf] = *(p+3+poff1);  // T
               ser_chunk[vpos+2*tf] = *(p+4+poff1);  // E
            }
			else{
               poff0 = (i-1) * 3;

               // LIDs must be strict increasing, Ts are not
               // Quantization errors must be <= descriptor size
               assert(*(p+2+poff1) > *(p+2+poff0));
               assert(*(p+3+poff1) >= *(p+3+poff0));
               assert(*(p+4+poff1) <= Pms::IDI);

               ser_chunk[vpos] = *(p+2+poff1) - *(p+2+poff0);
               ser_chunk[vpos+1*tf] = *(p+3+poff1) - *(p+3+poff0);
               // TODO: Find a way to delta-encode E ?
               ser_chunk[vpos+2*tf] = *(p+4+poff1);
            }
        }

        vpos += 2*tf;

        // Save FID[t-1] if delta-encoding is enabled
        prev_FID = delta_encode ? FID : 0;
    }

    // This should never throw. If it does, the bug is serious.
    assert(vpos == ser_chunk_size);

}

}// end namespace Audioneex
