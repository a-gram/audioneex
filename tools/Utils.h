/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

// Utility functions for internal use.

#ifndef UTILS_H
#define UTILS_H

#ifdef WIN32
 #include <Windows.h>
 #include <intrin.h>
#endif

#include <vector>
#include <list>
#include <ctime>
#include <fstream>
#include <sstream>
#include <random>
#include <climits>
#include <boost/algorithm/string.hpp>

#ifdef AUDIONEEX_API_EXPORT
 #include "BinaryVector.h"
 #include "Fingerprint.h"
#endif

/// Typedefs
typedef std::pair<std::string, std::string>  key_value_pair;

namespace Audioneex
{
namespace Utils
{

/// Hamming distance
inline uint32_t Dh(const uint8_t* str1, size_t str1_len,
                   const uint8_t* str2, size_t str2_len)
{
    assert(str1_len == str2_len);

    uint32_t v1=0, v2=0, xv=0, d=0;

    // Compute number of (32 bit) blocks and spare bytes
    uint32_t nb = str1_len / sizeof(uint32_t);
    uint32_t sb = str1_len % sizeof(uint32_t);

    for(uint32_t i=0; i<nb; i++){
        xv = ((uint32_t*)str1)[i] ^ ((uint32_t*)str2)[i];
#ifdef WIN32
        d += __popcnt(xv);
#else
        d += __builtin_popcount(xv);
#endif
    }

    // If the strings sizes are not integer multiple of the block size
    // perform Hamming distance on remaining bytes.
    // NOTE: this block of code can be removed if the strings size are
    //       integer multiple of the bit block size.
    if(sb){
        for(uint32_t i=0, j=str1_len-sb; i<sb; i++, j++){
           ((uint8_t*)&v1)[i] = str1[j];
           ((uint8_t*)&v2)[i] = str2[j];
        }
        xv = v1 ^ v2;
#ifdef WIN32
        d += __popcnt(xv);
#else
        d += __builtin_popcount(xv);
#endif
    }

    return d;
}

#ifdef AUDIONEEX_API_EXPORT

inline size_t Dh(BinaryVector &vec1, BinaryVector &vec2)
{
    assert(vec1.size() == vec2.size());
    assert(vec1.bcount() == vec2.bcount());
    return Dh(vec1.data(), vec1.bcount(),
              vec2.data(), vec2.bcount());
}

inline size_t Dh(BinaryVector *vec1, BinaryVector *vec2)
{
    assert(vec1->size() == vec2->size());
    assert(vec1->bcount() == vec2->bcount());
    return Dh(vec1->data(), vec1->bcount(),
              vec2->data(), vec2->bcount());
}

inline size_t Dh(LocalFingerprint_t &lf1, LocalFingerprint_t &lf2)
{
    assert(lf1.D.size() == lf2.D.size());
    return Dh(lf1.D.data(), lf1.D.size(),
              lf2.D.data(), lf2.D.size());
}

inline size_t Dh(LocalFingerprint_t *lf1, LocalFingerprint_t *lf2)
{
    assert(lf1->D.size() == lf2->D.size());
    return Dh(lf1->D.data(), lf1->D.size(),
              lf2->D.data(), lf2->D.size());
}

#endif

    // Timing functions

    inline double GetClockTime()
    {
#ifdef WIN32
        return static_cast<double>(GetTickCount64()) / 1000.0;
#else
        return 0;
#endif
    }

    inline double GetProcessTime()
    {
        return static_cast<double>(clock()) / CLOCKS_PER_SEC;
    }

#ifdef WIN32
	class HighResolutionTimer
	{
		LARGE_INTEGER m_freq;
		LARGE_INTEGER m_ticks;

	public:

		static const size_t MILLISECOND = 1000;
		static const size_t MICROSECOND = 1000000;

		HighResolutionTimer() 
		{
			int res = QueryPerformanceFrequency(&m_freq);
			if(res==0)
				throw std::runtime_error("QueryPerformanceFrequency() failed");
		}

		// Return time in specified unit
		template <size_t T>
		double now()
		{
			QueryPerformanceCounter(&m_ticks);
			return m_ticks.QuadPart * T / m_freq.QuadPart;
		}
	};
#endif

    // Other useful stuffs

    template<class T>
    inline std::string ToString(const T &val)
    {
        std::stringstream out;
        out << val;
        return out.str ();
    }

    template <class T>
    bool ToNumber(T& t, const std::string& s,
                  std::ios_base& (*f)(std::ios_base&) = std::dec)
    {
        std::istringstream iss(s);
        return !(iss >> f >> t).fail();
    }

    inline std::string FormatTime(int sec)
    {
        char buf[32];
        ::sprintf(buf, "%02d:%02d:%02d", sec/3600, (sec%3600)/60, (sec%3600)%60);
        return std::string(buf);
    }


    // Random number generators

    namespace rng
    {
        template <typename N>
        class natural
        {
            std::mt19937                       m_RNG;
            std::uniform_int_distribution<N>   m_RNG_int_udist;
          public:
            natural(N nmin = std::numeric_limits<N>::min(),
                    N nmax = std::numeric_limits<N>::max()) :
                m_RNG(std::random_device()()),
                m_RNG_int_udist(nmin, nmax)
            { }

            N get(){ return m_RNG_int_udist(m_RNG); }
            N operator()() { return get(); }
            N get_in(N nmin, N nmax) {
                m_RNG_int_udist.param(typename std::uniform_int_distribution<N>::param_type(nmin, nmax));
                return get();
            }
            N operator()(N nmin, N nmax) { return get_in(nmin, nmax); }
        };

        template <typename R>
        class real
        {
            std::mt19937                       m_RNG;
            std::uniform_real_distribution<R>  m_RNG_real_udist;
          public:
            real(R rmin = std::numeric_limits<R>::min(),
                 R rmax = std::numeric_limits<R>::max()) :
                m_RNG(std::random_device()()),
                m_RNG_real_udist(rmin, rmax)
            { }

            R get(){ return m_RNG_real_udist(m_RNG); }
            R operator()() { return get(); }
            R get_in(R rmin, R rmax) {
                m_RNG_real_udist.param(typename std::uniform_real_distribution<R>::param_type(rmin, rmax));
                return get();
            }
            R operator()(R rmin, R rmax) { return get_in(rmin, rmax); }
        };
    }// rng


    // log2(x)
    inline double log2(double x)
	{
        return log(x)/log(2.0);
    }

} //end namespace Utils

}// end namespace Audioneex

#endif // UTILS_H
