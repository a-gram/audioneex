
About
=====

Audioneex is an audio content recognition (ACR) engine providing audio fingerprinting 
technology specifically designed for real-time applications. It is general purpose, 
based on content-agnostic algorithms and runs on all kinds of machines, from big 
servers to mobile and embedded devices.


What it is for
--------------

ACR systems can be used in a variety of scenarios, such as broadcast monitoring, 
over-the-air (OTA) identification, content synchronization, second screen, audio 
surveillance, etc. Audio content identification and management technology finds 
applications in a wide range of industries. Following are a few examples of the 
most common use cases:

* User engagement
* Copyright management
* Advertisement
* Piracy detection
* Law enforcement
* Audience metering
* Surveillance & Safety

Audioneex provides the core technology for such applications in the form of a 
C++ cross-platform API that can be integrated as a backend component in web 
services, mobile and desktop apps, embedded systems and more.


Features
--------

* *Highly efficient fingerprinting* - The extraction of fingerprints is much faster 
  than real-time even on commodity hardware and the resulting fingerprints have a 
  low footprint. On average, one hour of audio will be encoded into a fingerprint 
  of less than 1 MB (in uncompressed form).
  
* *Fast response times* - Typical recognition times are in the order of a few 
  seconds (3-4 seconds for non-distorted or moderately distorted audio) making it 
  suitable for real-time applications.
  
* *Cross-platform implementation* - The engine has been developed in C++ in order
  to guarantee a highly performant native experience. It builds and runs anywhere
  there is a modern C++ compiler (version 11 and above), which means any 
  software/hardware platform out there.

* *Content-agnostic recognition* - The core algorithms are independent of the 
  nature of the audio to be identified allowing the identification of basically 
  any kind of content, from music, to TV/Radio shows, movies, commercials, 
  news and even generic sounds.
  
* *Highly flexible recognition system* - By providing several parameters that can
  be set through the API, the engine allows fine-tuning of performances based on 
  the kind of application at hand. 


Architecture
------------

The architecture is extremely modular, with three main interfaces that provide 
access to most of the functionality of the engine: ``Recognizer``, ``Indexer`` and 
``DataStore``.

.. figure:: _static/arch.png

The ``Recognizer`` can be considered as the front-end to all of the identification 
functionality. It deals with the collection of the audio supplied by the clients, 
dispatching of the audio to the ``Fingerprinter`` for fingerprint extraction, 
dispatching of the extracted fingerprints to the ``Matcher`` to initiate the search 
of the best candidates, analysis of the results returned by the matching process 
and production of the final results.

The ``Indexer`` provides access to all of the functionality regarding the production 
of the reference fingerprints. It deals with the collection of the audio from 
client applications, initiating the fingerprinting process, processing the 
resulting fingerprints into a format suitable for quick searches and storing the 
data into the appropriate structures. In the context of the Audioneex engine, 
all these processes collectively are referred to as “indexing”, and the outcome 
is the production of the “reference database”, which is the first step to take 
before using the engine for any recognition operation.

The ``Datastore`` interface is probably the most important entity of the whole 
architecture from the developer's point of view. It provides the specifications 
that clients can follow in order to interface the engine with a specific data 
store. It's an abstraction layer that can be used to write "drivers" for 
different kinds of database. 
This approach allows decoupling from vendor-specific solutions, with the 
consequence of providing a lot of flexibility in the choice of this crucial
component depending on the application. 
For example, a web service may need to use a database based on a client-server 
architecture, whereas an embedded system may require an in-process database for 
on-device recognitions. Data access drivers for two popular high-performance 
databases are provided out-of-the-box and can be used straight away.

