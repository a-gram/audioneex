/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef BLOCKCODEC_H
#define BLOCKCODEC_H

#include <cstdint>
#include <cassert>

// The following classes are not part of the public API but we need
// their interfaces exposed when testing DLLs.
#ifdef TESTING
  #include "audioneex.h"
  #define AUDIONEEX_API_TEST AUDIONEEX_API
#else
  #define AUDIONEEX_API_TEST
#endif


namespace Audioneex
{


/**
 * Part of the code in the following class was derived from code in the FastPFor
 * package by Daniel Lemire.
 *
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 * (c) Daniel Lemire, http://lemire.me/en/
 */

/// An integer array codec used to compress/decompress postings lists

class AUDIONEEX_API_TEST VByteCODEC
{
public:
    /// Compress the input integer array v using vByte encoding and puts the
    /// resulting byte stream in the output byte array venc.
    /// NOTE: Enough memory must be allocated for venc in order to host the encoded
    /// byte stream. An estimate may be calculated considering that in the worst case
    /// each integer element would require Nb = ceil( sizeof(int_type) * 8 / 7 ) bytes.
    /// Returns the number of bytes used to encode the input array.
    size_t encode(const uint32_t* v, size_t v_size, uint8_t* venc, size_t venc_size)
    {
        assert(v && venc);

        uint8_t * bout = venc;
        const uint8_t * const initbout = venc;

        for (size_t k = 0; k < v_size; ++k)
        {
            const uint32_t val(v[k]);

            if (val < (1U << 7)) {
                *bout = static_cast<uint8_t>(val | (1U << 7));
                ++bout;
            } else if (val < (1U << 14)) {
                *bout = static_cast<uint8_t>((val >> (7 * 0)) & ((1U << 7) - 1));
                ++bout;
                *bout = static_cast<uint8_t>((val >> (7 * 1))) | (1U << 7);
                ++bout;
            } else if (val < (1U << 21)) {
                *bout = static_cast<uint8_t>((val >> (7 * 0)) & ((1U << 7) - 1));
                ++bout;
                *bout = static_cast<uint8_t>((val >> (7 * 1)) & ((1U << 7) - 1));
                ++bout;
                *bout = static_cast<uint8_t>((val >> (7 * 2))) | (1U << 7);
                ++bout;
            } else if (val < (1U << 28)) {
                *bout = static_cast<uint8_t>((val >> (7 * 0)) & ((1U << 7) - 1));;
                ++bout;
                *bout = static_cast<uint8_t>((val >> (7 * 1)) & ((1U << 7) - 1));
                ++bout;
                *bout = static_cast<uint8_t>((val >> (7 * 2)) & ((1U << 7) - 1));
                ++bout;
                *bout = static_cast<uint8_t>((val >> (7 * 3))) | (1U << 7);
                ++bout;
            } else {
                *bout = static_cast<uint8_t>((val >> (7 * 0)) & ((1U << 7) - 1));
                ++bout;
                *bout = static_cast<uint8_t>((val >> (7 * 1)) & ((1U << 7) - 1));
                ++bout;
                *bout = static_cast<uint8_t>((val >> (7 * 2)) & ((1U << 7) - 1));
                ++bout;
                *bout = static_cast<uint8_t>((val >> (7 * 3)) & ((1U << 7) - 1));
                ++bout;
                *bout = static_cast<uint8_t>((val >> (7 * 4))) | (1U << 7);
                ++bout;
            }
        }

        return static_cast<size_t>(bout - initbout);
    }

    /// Decompress the input byte stream v using vByte decoding and puts the decoded
    /// integer elements in the output array vdec.
    /// NOTE: Enough memory must be allocated for vdec in order to host the decoded
    ///       elements. An estimate may be calculated considering that in the best
    ///       case each integer element has been compressed using 1 byte, so the max
    ///       number of decompressed elements will be v.size (thus worst_case = v.size)
    /// Returns the number of integer elements decoded from the input stream.
    size_t decode(const uint8_t* v, size_t v_size, uint32_t* vdec, size_t vdec_size)
    {
        assert(v && vdec);

        const uint8_t * inbyte = v;
        const uint8_t * const endbyte = v + v_size;
        const uint32_t * const initout(vdec);
        uint32_t * iout = vdec;

        while (endbyte > inbyte)
		{
            unsigned int shift = 0;

            for (uint32_t e = 0; endbyte > inbyte; shift += 7)
			{
                uint8_t c = *inbyte++;
                e += ((c & 127) << shift);
				
                if ((c & 128))
				{
                    if(iout >= vdec + vdec_size)
                       return 0; //<- or throw ?
                    *iout++ = e;
                    break;
                }
            }
        }
        return static_cast<size_t>(iout - initout);
    }

};

// ----------------------------------------------------------------------------

/// The BlockEncoder is responsible for the transformation of postings lists
/// into byte streams that can be stored somewhere. Postings lists chunks
/// are first serialized into arrays of integers with specific layout and then
/// delta-encoded and compressed into a stream of bytes.
class AUDIONEEX_API_TEST BlockEncoder
{
    VByteCODEC            m_Codec;
    std::vector<uint32_t> m_ser_chunk;

public:

    /// Delta-codec constants
    enum{
        DENCODE = -1,
        DDECODE = 1
    };

    /// Encode the given postings list chunk into a byte stream.
    /// The 'FIDo' parameter indicates the base value from which
    /// the delta encoding of the FIDs will be computed.
    /// @return  Zero if no errors occurred.
    int Encode(const uint32_t* const* plist_chunk,
                size_t plist_chunk_size,
                uint8_t* enc_chunk,
                size_t enc_chunk_size,
                size_t& enc_bytes,
                uint32_t FIDo=0,
                bool delta_encode=true);

    /// Decode the given byte stream into an array of integers.
    /// The 'FIDo' parameter indicates the base value from which
    /// the delta decoding of the FIDs will be computed.
    /// @return  Zero if no errors occurred.
    int Decode(const uint8_t *enc_chunk,
                size_t enc_chunk_size,
                uint32_t *dec_chunk,
                size_t dec_chunk_size,
                size_t& dec_elem,
                uint32_t base_FID=0,
                bool delta_decode=true);

    void Serialize(const uint32_t* const* plist_chunk,
                   size_t plist_chunk_size,
                   uint32_t *ser_chunk,
                   size_t ser_chunk_size,
                   uint32_t prev_FID=0,
                   bool delta_encode=true);

    /// Compute an estimate of the worst case when decoding an encoded
    /// integer array, that is the max size that the encoded array can
    /// take after decoding. This is useful to properly allocate the
    /// decoding buffer and avoid nasty surprises. The possibility of
    /// getting a precise estimate depends on the encoding algorithm.
    static inline size_t GetDecodedSizeEstimate(size_t enc_size){
        return enc_size;
    }

    /// Compute an estimate of the worst case when encoding an int
    /// array, that is the max size that the integer array can
    /// take after encoding. This is useful to properly allocate the
    /// encoding buffer and avoid nasty surprises. The possibility of
    /// getting a precise estimate depends on the encoding algorithm.
    static inline size_t GetEncodedSizeEstimate(size_t dec_size){
        return dec_size * (sizeof(uint32_t)+1);
    }

    /// Apply delta-encoding/decoding to the given int array.
    /// Return false if something wrong occurs, true otherwise.
    template<int T>
    bool DeltaCodec(uint32_t* chunk, size_t csize, uint32_t base_FID=0)
    {
        uint32_t* begin = csize==0 ? nullptr : chunk;
        uint32_t* end   = begin + csize;
        uint32_t bfid   = base_FID;
        uint32_t tf,i,j;

        while(begin < end)
		{
            *begin += bfid;
            bfid = *begin;
            begin++;
            tf = *begin;
            begin++;
            // NOTE: The quantization errors are not d-encoded
            for(j=1; j<=2; j++, begin++)
               for(i=tf-1; i; begin++, i--){
                   if(begin+1 >= end) return false;
                  *(begin+1) += *begin*T;
               }

            begin+=tf;
        }
        return true;
    }

};

}// end namespace Audioneex

#endif
