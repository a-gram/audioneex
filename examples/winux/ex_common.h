/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef EXCOMMON_H
#define EXCOMMON_H

#include "audioneex.h"

// Define the datastore implementation to be used (TCDataStore/CBDataStore)

#define DATASTORE_T_TC 1
#define DATASTORE_T_CB 2

#ifndef DATASTORE_T_ID
 #error "DATASTORE_T_ID not defined."
#endif

#ifndef DATASTORE_T
 #error "DATASTORE_T not defined. Please add a DATASTORE_T=TCDataStore|CBDataStore compiler definition to your build system."
#endif

class DATASTORE_T;  //< Must be defined somewhere

/// Interfaces used in the examples

class IdentificationResultsListener{
 public:
    virtual void OnResults(const Audioneex::IdMatch* results) = 0;
};


#endif // EXCOMMON_H
