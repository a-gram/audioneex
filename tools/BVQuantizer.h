/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef BINARY_VECTOR_QUANTIZER_H
#define BINARY_VECTOR_QUANTIZER_H

#include <cstdint>
#include <cassert>
#include <array>
#include <string>
#include <vector>

#include "BinaryVector.h"
#include "Codebook.h"

typedef std::array<uint32_t, 2>  array_2D;

namespace Audioneex
{

/// A binary vector quantizator based on a k-medians algorithm.

class BVQuantizer
{
    int                         m_K;
    std::vector<BinaryVector>   m_Points;
    std::vector<Cluster>        m_Clusters;

    void KmeansPP();
    void RandomSeeding();

  public:

    BVQuantizer(int K);
   ~BVQuantizer() = default;

    void addPoint(BinaryVector &point);

    std::shared_ptr <Codebook> Kmedians();

    BinaryVector& point(size_t n) { return m_Points[n]; }
    size_t npoints()              { return m_Points.size(); }

};

}// end namespace Audioneex

#endif // BINARYVECTORQUANTIZER_H

