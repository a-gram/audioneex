/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/


package com.audioneex.recognition;

public class RecognitionServiceException extends Exception {

    public RecognitionServiceException(String message) {
        super(message);
    }

    public RecognitionServiceException(String message, Throwable throwable) {
        super(message, throwable);
    }
}

