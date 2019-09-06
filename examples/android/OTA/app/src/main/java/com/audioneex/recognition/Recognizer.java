/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/


package com.audioneex.recognition;

class Recognizer {

	native void  SetMatchType(int type);
	native int   GetMatchType();
	native void  SetIdentificationType(int type);
	native int   GetIdentificationType();
	native void  SetIdentificationMode(int mode);
	native int   GetIdentificationMode();
	native void  SetBinaryIdThreshold(float value);
	native float GetBinaryIdThreshold();
	native boolean Identify(float[] audioclip, int nsamples);
	native String GetResults();
	native void   Reset();
	
}
