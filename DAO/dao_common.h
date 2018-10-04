/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef DAOCOMMON_H
#define DAOCOMMON_H

// The following definitions fix names clashes on Windows
#ifdef WIN32
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
#endif

// Define the supported data store backends that can be used.
// Currently supported backends are Tokyo Cabinet and Couchbase.

#define DATASTORE_T_TC 1
#define DATASTORE_T_CB 2

#ifndef DATASTORE_T_ID
 #error "DATASTORE_T_ID not defined. You must add a DATASTORE_T_ID \
         definition to your build system to specify the data store id. \
         Accepted values are DATASTORE_T_ID=DATASTORE_T_TC|DATASTORE_T_CB."
#endif

#ifndef DATASTORE_T
 #error "DATASTORE_T not defined. You must add a DATASTORE_T \
         definition to your build system to specify the data store to be used. \
         Accepted values are DATASTORE_T=TCDataStore|CBDataStore."
#endif

#if defined(DATASTORE_T_ID) && (DATASTORE_T_ID==DATASTORE_T_TC)
  #include "TCDataStore.h"
#elif defined(DATASTORE_T_ID) && (DATASTORE_T_ID==DATASTORE_T_CB)
  #include "CBDataStore.h"
#else
  #error "Undefined datastore"
#endif


#endif // DAOCOMMON_H
