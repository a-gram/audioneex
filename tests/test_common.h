/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef TESTCOMMON_H
#define TESTCOMMON_H

#include <vector>
#include <algorithm>
#include <cmath>

#include "audioneex.h"

template<typename C, typename V>
bool REQUIRE_IN(C& cont, V value)
{
    return std::find(begin(cont), end(cont), value) != end(cont);
}

template<typename V>
bool REQUIRE_IN_RANGE(std::vector<V> range, V value)
{
    return range[0] <= value && value <= range[1];
}

bool REQUIRE_ALMOST_EQUAL(double comp, 
                          double val, 
                          double delta=1E-9)
{
    return std::abs(val - comp) <= delta;
}

#endif
