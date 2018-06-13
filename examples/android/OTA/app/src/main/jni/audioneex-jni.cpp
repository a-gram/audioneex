/*

Copyright (c) 2014, Audioneex.com.
Copyright (c) 2014, Alberto Gramaglia.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list
   of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this
   list of conditions and the following disclaimer in the documentation and/or other
   materials provided with the distribution.

3. The name of the author(s) may not be used to endorse or promote products derived
   from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.

*/

#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <cstdint>
#include <memory>
#include <map>

#include <jni.h>
#include <android/log.h>

#define  LOG_TAG    "audioneex-jni"

#define  LOG_D(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__);
#define  LOG_E(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);

#include "audioneex-jni.h"

// JNI interface

extern "C"
{
    JNIEXPORT jboolean JNICALL Java_com_audioneex_recognition_RecognitionService_Initialize(JNIEnv *env, jclass clazz, jstring datastoreDir);
    JNIEXPORT jboolean JNICALL Java_com_audioneex_recognition_Recognizer_Identify(JNIEnv *env, jclass clazz, jfloatArray audio, jint audiolen);
    JNIEXPORT jstring JNICALL Java_com_audioneex_recognition_Recognizer_GetResults(JNIEnv *env, jclass clazz);
    JNIEXPORT void JNICALL Java_com_audioneex_recognition_Recognizer_Reset(JNIEnv *env, jclass clazz);
    JNIEXPORT void JNICALL Java_com_audioneex_recognition_Recognizer_SetBinaryIdThreshold(JNIEnv *env, jclass clazz, jfloat value);
    JNIEXPORT jfloat JNICALL Java_com_audioneex_recognition_Recognizer_GetBinaryIdThreshold(JNIEnv *env, jclass clazz);

}

// Internal helpers

std::string ResultsToJSON(const Audioneex::IdMatch* results);

// Implementation

jboolean Java_com_audioneex_recognition_RecognitionService_Initialize(JNIEnv *env,
                                                                      jclass clazz,
                                                                      jstring datastoreDir)
{
    try
    {
    	const char *ddir_c = env->GetStringUTFChars(datastoreDir, NULL);
	if (NULL == ddir_c)
	    throw std::runtime_error("Couldn't get C string from JNI");
        std::string dstoreDir = ddir_c;
        env->ReleaseStringUTFChars(datastoreDir, ddir_c);
        LOG_D("JNI Init: Datastore dir: %s", dstoreDir.c_str())
        LOG_D("JNI Init: AUDIONEEX ENGINE VERSION: %s", Audioneex::GetVersion())
        ACIEngine::instance().init(dstoreDir);

    }
    catch(const std::exception &ex)
    {
        LOG_E("NATIVE EXCEPTION [RecognitionService.Initialize()]: %s", ex.what())
        return false;
    }
    return true;
}

jboolean Java_com_audioneex_recognition_Recognizer_Identify(JNIEnv *env,
                                                            jclass clazz,
                                                            jfloatArray audio,
                                                            jint audiolen)
{
    try
    {
    	jfloat *audio_c = env->GetFloatArrayElements(audio, NULL);
        jsize bufferlen = env->GetArrayLength(audio);

	if (NULL == audio_c)
	   throw std::runtime_error("Couldn't get C array from JNI");
	if(audiolen > bufferlen)
	   throw std::runtime_error("Invalid audio clip length");

        LOG_D("JNI Identify: Identifying clip of %d samples", audiolen)
        
        ACIEngine::instance().recognizer()->Identify(audio_c, audiolen);
        env->ReleaseFloatArrayElements(audio, audio_c, 0);
    }
    catch(const std::exception &ex)
    {
        LOG_E("EXCEPTION [Recognizer.Identify()]: %s", ex.what())
        return false;
    }
    return true;
}

jstring Java_com_audioneex_recognition_Recognizer_GetResults(JNIEnv *env, jclass clazz)
{
    try
    {
        const Audioneex::IdMatch* results = ACIEngine::instance().recognizer()->GetResults();

        if(results) {
           std::string json = ResultsToJSON(results);
           jstring ret = env->NewStringUTF(json.c_str());
           LOG_D("ID RESULTS: %s", json.c_str())
           return ret;
        }
    }
    catch(const std::exception &ex)
    {
        LOG_E("EXCEPTION [Recognizer.GetResults()]: %s", ex.what())
        std::string error = "{ \"status\":\"ERROR\",\"message\":\""+std::string(ex.what())+"\"}";
        jstring ret = env->NewStringUTF(error.c_str());
        return ret;
    }
    return NULL;
}

void Java_com_audioneex_recognition_Recognizer_Reset(JNIEnv *env, jclass clazz)
{
    try
    {
        ACIEngine::instance().recognizer()->Reset();
    }
    catch(const std::exception &ex)
    {
        LOG_E("EXCEPTION [Recognizer.GetResults()]: %s", ex.what())
    }
}

void Java_com_audioneex_recognition_Recognizer_SetBinaryIdThreshold(JNIEnv *env, jclass clazz, jfloat value)
{
    try
    {
        ACIEngine::instance().recognizer()->SetBinaryIdThreshold(value);
    }
    catch(const std::exception &ex)
    {
        LOG_E("EXCEPTION [Recognizer.SetBinaryIdThreshold()]: %s", ex.what())
    }
}

jfloat Java_com_audioneex_recognition_Recognizer_GetBinaryIdThreshold(JNIEnv *env, jclass clazz)
{
    try
    {
        return ACIEngine::instance().recognizer()->GetBinaryIdThreshold();
    }
    catch(const std::exception &ex)
    {
        LOG_E("EXCEPTION [Recognizer.GetBinaryIdThreshold()]: %s", ex.what())
    }
    return UNSPECIFIED_ERROR;
}


std::string ResultsToJSON(const Audioneex::IdMatch* results)
{
    std::stringstream ss;
    std::map<int, std::string> idclass;

    ss << "{ \"status\":\"OK\", \"Matches\":[";

    for(int i=0; !Audioneex::IsNull(results[i]); i++){
    	std::string meta = ACIEngine::instance().datastore()->GetMetadata(results[i].FID);
    	ss << (i>0?",":"")
    	   << "{"
           << "\"FID\":" << results[i].FID << ","
           << "\"Score\":" << results[i].Score << ","
           << "\"Confidence\":" << results[i].Confidence << ","
           << "\"Metadata\":" << "\"" << meta << "\""
           << "}";
    }
    ss << "]}";
    return ss.str();
}

