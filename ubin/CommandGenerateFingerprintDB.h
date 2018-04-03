/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef COMMANDGENERATEFINGERPRINTDATABASE_H
#define COMMANDGENERATEFINGERPRINTDATABASE_H

#include "common.h"
#include "Command.h"
#include "QFGenerator.h"
#include "TCDataStore.h"


/// Execute  --generate-fingerprint-db [options]

class CommandGenerateFingerprintDB : public Command
{
	size_t       m_FingNum;
	std::string  m_DBURL;
	

	void Generate()
	{
		TCDataStore data_store( m_DBURL );
		data_store.Open(KVDataStore::BUILD, true, true);
		
        std::unique_ptr<Audioneex::Indexer> indexer ( Audioneex::Indexer::Create() );
		indexer->SetDataStore( &data_store );
		indexer->SetMatchType( Audioneex::MSCALE_MATCH );
		indexer->Start();

		QFGenerator  fingerprint;

		for(size_t FID=1; FID<=m_FingNum; FID++)
		{
			// Generate a random fingerprint
			std::vector<Audioneex::QLocalFingerprint_t>& fp = fingerprint.Generate();

            if(fp.empty())
               throw std::runtime_error("Invalid fingerprint (null)");

            const uint8_t* fp_ptr = reinterpret_cast<uint8_t*>(fp.data());
            size_t   fp_bytes = fp.size() * sizeof(Audioneex::QLocalFingerprint_t);

			// Index the fingerprint
            indexer->Index(FID, fp_ptr, fp_bytes);

			// Save the fingerprint
            static_cast<KVDataStore*>(&data_store)->PutFingerprint(FID,fp_ptr,fp_bytes);

			static_cast<KVDataStore*>(&data_store)->PutMetadata( FID, "ID" );

			DEBUG_MSG("FID="<<FID<<", NLFS="<<fp.size()<<", Len.: ~"<<(fp[fp.size()-1].T*Audioneex::Pms::dt)<<"s")

		}

		indexer->End();
	}


public:

    CommandGenerateFingerprintDB() :
		m_FingNum    (1000),
		m_DBURL      ("")
	{
        m_Usage = "\n\nSyntax: --generate-fingerprint-db [options]\n\n";
		m_Usage += "     Options:\n\n";
		m_Usage += "     -n = number of fingerprints (default=1000)\n";
		m_Usage += "     -u = database url (default=.)\n";

		m_SupportedArgs.insert("-n");
		m_SupportedArgs.insert("-u");
    }

    void Execute()
    {
        if(!ValidArgs())
           PrintUsageAndThrow("Invalid arguments.");

		GetArgValue("-n", m_FingNum);
		GetArgValue("-u", m_DBURL);

		Generate();
    }

};

#endif
