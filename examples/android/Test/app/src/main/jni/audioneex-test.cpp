/*
    Copyright (c) 2014 Audioneex.com. All rights reserved.
	
	This source code is part of the Audioneex software package and is
	subject to the terms and conditions stated in the accompanying license.
	Please refer to the Audioneex license document provided with the package
	for more information.
	
	Author: Alberto Gramaglia
	
*/

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <cstdint>
#include <memory>

#include <jni.h>
#include <android/log.h>

#define  LOG_TAG    "TestApp"

#define  LOG_D(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__);
#define  LOG_E(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);

#include "audioneex-test.h"

extern "C" {
    JNIEXPORT jboolean JNICALL Java_com_audioneex_test_TestApp_doTest(JNIEnv* env, jclass clazz, jstring dataDir);
};

jboolean Java_com_audioneex_test_TestApp_doTest(JNIEnv* env, jclass clazz,
                                            jstring dataDir)
{
    try
    {
        const char *utf8 = env->GetStringUTFChars(dataDir, NULL);
        assert(utf8 != nullptr);

	    std::string pwd = utf8;

	    env->ReleaseStringUTFChars(dataDir, utf8);

        std::unique_ptr<KVDataStore> dstore ( new TCDataStore (pwd+"/data") );
        dstore->Open( KVDataStore::BUILD );

		// Let's index some recordings ...

        IndexingTask itask (pwd+"/audio");

        std::unique_ptr<Audioneex::Indexer> indexer ( Audioneex::Indexer::Create() );
        indexer->SetDataStore( dstore.get() );
        indexer->SetAudioProvider( &itask );

        itask.SetIndexer( indexer.get() );
        itask.Run();

        dstore->Close();

        // ... and now let's identify them

        IdentificationTask idTask;

        dstore->Open( KVDataStore::GET );

        std::unique_ptr<Audioneex::Recognizer> recognizer ( Audioneex::Recognizer::Create() );
        recognizer->SetDataStore( dstore.get() );

        idTask.SetRecognizer( recognizer.get() );

        for(uint32_t i=1; i<=TEST_REC_NUM; i++)
        {
			std::string afile = pwd+"/audio/rec"+n_to_string(i)+".wav";
            idTask.GetAudioSource().SetPosition( i*5 );
            if( idTask.Identify( afile ) != i )
               throw std::logic_error("Couldn't identify "+afile);
        }

        LOG_D("TEST PASSED")
    }
    catch(const std::exception &ex){
        LOG_E("TEST FAILED: %s", ex.what())
        return false;
    }

    return true;
}

