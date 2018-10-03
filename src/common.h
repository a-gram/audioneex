/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef COMMON_H
#define COMMON_H

#ifdef WIN32
 #define NOMINMAX
#endif

#include <memory>
#include <iostream>

/// Defines

#define DEBUG_MSG(msg)    std::cout << msg << std::endl;
#define DEBUG_MSG_F(msg)  std::cout << msg << std::endl; std::cout.flush();
#define DEBUG_MSG_N(msg)  std::cout << msg;
#define DEBUG_MSG_NF(msg) std::cout << msg; std::cout.flush();

#define WARNING_MSG(msg)  std::cout << "WARNING: " << msg << std::endl;
#define ERROR_MSG(msg)    std::cerr << "ERROR: " << msg << std::endl;

#ifdef TESTING
 #define TEST_HERE( test_code ) test_code
#else
 #define TEST_HERE( test_code )
#endif

#endif // COMMON_H
