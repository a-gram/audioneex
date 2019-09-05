/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/


#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <cmath>
#include <cstdint>

/// This namespace contains parameters used internally by the algorithms.
/// Most of them need not to be changed, and for those that can be, make
/// sure you know what you are doing.

namespace Audioneex
{
namespace Pms
{

const int      Fmin            = 100;
const int      Fmax            = 3100; // Must be in ]Fmin, Fs/2]

const double   Fs              = 11025;
const int      Ca              = 1; // Audio channels

const int      OrigWindowSize  = 1024;     // wanted size
const int      windowSize      = 2048;     // actual size, after zero-padding
const float    zeroPadFactor   = static_cast<float>(windowSize) /
                                 static_cast<float>(OrigWindowSize) - 1.f;
                                 
const double   hopInterval     = 0.0138776; // in seconds
const int      hopSize         = hopInterval * Fs;
const double   df              = Fs / windowSize;
const double   dt              = hopInterval;
const int      Kmin            = floor(windowSize*Fmin/Fs);
const int      Kmax            = floor(windowSize*Fmax/Fs);

// The optimal time span value for Wp appears to be within [0.400, 0.500]
const float    dTWp = 0.400;     // Peak's neighborhood time span for non-maximum suppression (in s)
const float    dFWp = 340;       // Peak's neighborhood frequency span for non-maximum suppression  (in Hz)
const float    dTNp = 0.300;     // POI's neighborhood time span (in s)
const float    dFNp = 200;       // POI's neighborhood frequency span (in Hz)
const float    dTWc = 0.050;     // Scanning window time span (in s)
const float    dFWc = 35;        // Scanning window frequency span (in Hz)
const float    sf   = 50;        // Scanning window frequency stride (in % of dFWc)
const float    st   = 50;        // Scanning window time stride (in % of dTWc)
const float    bf   = 50;        // Neighboring window frequency displacement (in % of dFWc)
const float    bt   = 50;        // Neighboring window time displacement (in % of dTWc)
// tried qT=2 qF=5, working ok. qt=5 qF=9 seems to work best(?)
const float    qT   = 5;         // Time quantization step (in s)
const float    qF   = 9;         // Frequency quantization step (in Hz)

// radius of Wp in t-f units (frames-bins)
const int      rWp  = (dTWp/2) / dt;
const int      rHp  = (dFWp/2) / df;

// radius of N(p) in t-f units (frames-bins)
const int      rNpF = (dFNp/2) / df;
const int      rNpT = (dTNp/2) / dt;

// radius of scanning window Wc in t-f units (frames-bins)
const int      rWcF = (dFWc/2) / df;
const int      rWcT = (dTWc/2) / dt;

// convert Wc strides in t-f units
const int      nsf  = ((sf/100.f) * dFWc) / df;
const int      nst  = ((st/100.f) * dTWc) / dt;
const int      nbf  = ((bf/100.f) * dFWc) / df;
const int      nbt  = ((bt/100.f) * dTWc) / dt;

// Number of scanning windows along time and frequency in N(p)
const int      nWcF = ((rNpF*2+1) - (rWcF*2+1)) / nsf;
const int      nWcT = ((rNpT*2+1) - (rWcT*2+1)) / nst;

// Number of scanning windows in N(p)
const int      nWc  = nWcT * nWcF;

// Size of descriptor in bits (rounded to the highest byte)
const int      IDI  = ceil(4.0 * nWc / 8.0) * 8;

// Size of descriptor in bytes
const int      IDI_b = IDI / 8;

// ----------------------------------------------------------------------------
//   The following parameters could be user-adjustable rather than constants
// ----------------------------------------------------------------------------

/// The score assigned by the ranking systems. It is normally weighed by
/// values in the range [0,1], so this is the max assignable score.
const int Smax = 1000;

/// The K parameter of the k-medians algorithm (# of codewords)
const int Kmed = 100;

/// The minimum number of LF to be processed at each matching step.
/// This value represents the minimum amount of evidence needed in order
/// to start a matching step. Actually set to 20, roughly corresponding
/// to 1 sec of audio with the current settings.
const int Nk = 20;

/// The size of a time bin ('listening quantum') in spectral time units.
/// A 'listening quantum' is the shortest period of time needed for
/// recognizing an audio event (or audio scene). It may range from
/// 1 second or less to few seconds (3-5) and is application dependent.
/// This is actually set to 365 time units, which, with the current spectrum
/// time resolution, is equivalent to about 5 seconds of audio.

/// NOTE: The smaller this value the fewer descriptors will fall into
///       the time bins during the candidates search and thus the lower
///       the evidence we can collect, which can affect the accuracy in
///       noisy audio where there may be only few matching LFs in the quantum,
///       but the faster the recognition time.
///       On the other hand, the bigger the value the more LFs will fall
///       in the bins of the Mc increasing the probability of capturing
///       enough evidence and get a clear peak but the slower the recognition.
///       DO NOT confuse this parameter with 'Nk'. Apparently similar, they
///       represent 2 different things: Nk being the minimum amount of evidence
///       necessary at each step to start the matching procedure (matching is
///       performed on blocks of LFs), Tk being the minimum time needed to
///       recognize an audio scene. The 2 values might be the same in some
///       applications, but they represent different things.
const int Tk = 365;

/// The number of LF to be matched in the t-f coherence ranking.
/// This parameter will determine the size of the sub-graphs G(Xk, Ek)
/// and G(Qh, Eh) being matched for each candidate found in the similarity
/// search stage.

/// NOTE: The bigger this value the bigger the sub-graphs used for matching
///       and thus the higher the probability of getting a good ranking, but
///       the slower the process since the time to build the hashtables used
///       for graph matching (and the size of these) will increase.
///       On the other hand, the smaller this value the smaller the sub-graphs
///       being matched, decreasing the probability of good ranking, but the
///       faster the process.
const int Ntf = 30;

/// Number of best candidates to be chosen in the matching stage.
/// This is the size of the top-k list used in the matching.
const int TopK = 20;

/// Maximum identification time (in seconds).
const float MaxIdTime = 20;

/// Maximum recording length in seconds.
/// Recordings that are too long may produce index list blocks much beyond the
/// set limit (POSTINGSLIST_BLOCK_THRESHOLD) and affect performance. Recordings
/// that are longer than this limit should be split into "parts".
const int MaxRecordingLength = 1800;

// -------------------- Functions -----------------------


/// Get the number of spectral channels
inline int GetChannelsCount()
{
    return ceil((Kmax - Kmin + 1) / qF);
}


}// end namespace Pms::
}// end namespace Audioneex

#endif // PARAMETERS_H
