/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
    JNIEXPORT void JNICALL Java_com_audioneex_recognition_Recognizer_SetMatchType(JNIEnv *env, jclass clazz, jint mtype);
    JNIEXPORT jint JNICALL Java_com_audioneex_recognition_Recognizer_GetMatchType(JNIEnv *env, jclass clazz);
    JNIEXPORT void JNICALL Java_com_audioneex_recognition_Recognizer_SetIdentificationType(JNIEnv *env, jclass clazz, jint idtype);
    JNIEXPORT jint JNICALL Java_com_audioneex_recognition_Recognizer_GetIdentificationType(JNIEnv *env, jclass clazz);
    JNIEXPORT void JNICALL Java_com_audioneex_recognition_Recognizer_SetIdentificationMode(JNIEnv *env, jclass clazz, jint idmode);
    JNIEXPORT jint JNICALL Java_com_audioneex_recognition_Recognizer_GetIdentificationMode(JNIEnv *env, jclass clazz);
    JNIEXPORT void JNICALL Java_com_audioneex_recognition_Recognizer_SetBinaryIdThreshold(JNIEnv *env, jclass clazz, jfloat value);
    JNIEXPORT jfloat JNICALL Java_com_audioneex_recognition_Recognizer_GetBinaryIdThreshold(JNIEnv *env, jclass clazz);

}

// Internal helpers

std::string 
ResultsToJSON(const Audioneex::IdMatch* results);

// Implementation

jboolean 
Java_com_audioneex_recognition_RecognitionService_Initialize(JNIEnv *env,
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

jboolean 
Java_com_audioneex_recognition_Recognizer_Identify(JNIEnv *env,
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

jstring 
Java_com_audioneex_recognition_Recognizer_GetResults(JNIEnv *env, jclass clazz)
{
    try
    {
        auto results = ACIEngine::instance().recognizer()->GetResults();

        if(results)
        {
           auto json = ResultsToJSON(results);
           jstring ret = env->NewStringUTF(json.c_str());
           LOG_D("ID RESULTS: %s", json.c_str())
           return ret;
        }
    }
    catch(const std::exception &ex)
    {
        LOG_E("EXCEPTION [Recognizer.GetResults()]: %s", ex.what())
        auto error = std::string("{ \"status\":\"ERROR\",\"message\":\"") + 
                     std::string(ex.what()) + "\"}";
        jstring ret = env->NewStringUTF(error.c_str());
        return ret;
    }
    return NULL;
}

void 
Java_com_audioneex_recognition_Recognizer_Reset(JNIEnv *env, jclass clazz)
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

void 
Java_com_audioneex_recognition_Recognizer_SetMatchType(JNIEnv *env, 
                                                       jclass clazz, 
                                                       jint mtype)
{
    try
    {
        auto emtype = static_cast<Audioneex::eMatchType>(mtype);
        ACIEngine::instance().recognizer()->SetMatchType(emtype);
    }
    catch(const std::exception &ex)
    {
        LOG_E("EXCEPTION [Recognizer.SetMatchType()]: %s", ex.what())
    }
}

jint 
Java_com_audioneex_recognition_Recognizer_GetMatchType(JNIEnv *env, jclass clazz)
{
    try
    {
        return ACIEngine::instance().recognizer()->GetMatchType();
    }
    catch(const std::exception &ex)
    {
        LOG_E("EXCEPTION [Recognizer.GetMatchType()]: %s", ex.what())
    }
    return UNSPECIFIED_ERROR;
}

void 
Java_com_audioneex_recognition_Recognizer_SetIdentificationType(JNIEnv *env, 
                                                                jclass clazz, 
                                                                jint idtype)
{
    try
    {
        auto eidtype = static_cast<Audioneex::eIdentificationType>(idtype);
        ACIEngine::instance().recognizer()->SetIdentificationType(eidtype);
    }
    catch(const std::exception &ex)
    {
        LOG_E("EXCEPTION [Recognizer.SetIdentificationType()]: %s", ex.what())
    }
}

jint 
Java_com_audioneex_recognition_Recognizer_GetIdentificationType(JNIEnv *env, 
                                                                jclass clazz)
{
    try
    {
        return ACIEngine::instance().recognizer()->GetIdentificationType();
    }
    catch(const std::exception &ex)
    {
        LOG_E("EXCEPTION [Recognizer.GetIdentificationType()]: %s", ex.what())
    }
    return UNSPECIFIED_ERROR;
}

void 
Java_com_audioneex_recognition_Recognizer_SetIdentificationMode(JNIEnv *env, 
                                                                jclass clazz, 
                                                                jint idmode)
{
    try
    {
        auto eidmode = static_cast<Audioneex::eIdentificationMode>(idmode);
        ACIEngine::instance().recognizer()->SetIdentificationMode(eidmode);
    }
    catch(const std::exception &ex)
    {
        LOG_E("EXCEPTION [Recognizer.SetIdentificationMode()]: %s", ex.what())
    }
}

jint 
Java_com_audioneex_recognition_Recognizer_GetIdentificationMode(JNIEnv *env, 
                                                                jclass clazz)
{
    try
    {
        return ACIEngine::instance().recognizer()->GetIdentificationMode();
    }
    catch(const std::exception &ex)
    {
        LOG_E("EXCEPTION [Recognizer.GetIdentificationMode()]: %s", ex.what())
    }
    return UNSPECIFIED_ERROR;
}

void 
Java_com_audioneex_recognition_Recognizer_SetBinaryIdThreshold(JNIEnv *env, 
                                                               jclass clazz, 
                                                               jfloat value)
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

jfloat 
Java_com_audioneex_recognition_Recognizer_GetBinaryIdThreshold(JNIEnv *env, 
                                                               jclass clazz)
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


std::string 
ResultsToJSON(const Audioneex::IdMatch* results)
{
    std::stringstream ss;
    std::map<int, std::string> idclass;

    idclass[Audioneex::UNIDENTIFIED] = "UNIDENTIFIED";
    idclass[Audioneex::SOUNDS_LIKE] = "SOUNDS_LIKE";
    idclass[Audioneex::IDENTIFIED] = "IDENTIFIED";

    ss << "{ \"status\":\"OK\", \"Matches\":[";

    for(int i=0; !Audioneex::IsNull(results[i]); i++)
    {
    	auto meta = ACIEngine::instance().datastore()->GetMetadata(results[i].FID);
    	ss << (i>0?",":"")
    	   << "{"
         << "\"FID\":" << results[i].FID << ","
         << "\"Score\":" << results[i].Score << ","
         << "\"Confidence\":" << results[i].Confidence << ","
         << "\"IdClass\":\"" << idclass[results[i].IdClass] << "\","
         << "\"Metadata\":" << "\"" << meta << "\""
         << "}";
    }
    ss << "]}";
    return ss.str();
}

