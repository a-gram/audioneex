/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/


package com.audioneex.audio;

public interface AudioSourceServiceListener
{
	public void SignalAudioSourceStart();
	public void SignalAudioSourceStop();
        public void SignalAudioBufferReady();
        public void SignalAudioSourceError(String error);
}
