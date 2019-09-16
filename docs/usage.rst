
How to use it
=============

To use the engine in your applications all you need to do is include the API 
header file ``audioneex.h`` and link to the shared library according to the 
platform used to build the application.

The first thing to do is fingerprinting some audio (a process here referred to 
as "indexing") by creating an instance of :class:`Indexer`, setting some 
properties, starting an indexing session and call the :meth:`Indexer::Index()` 
method, as shown in the following code

.. code-block:: cpp

   #include <audioneex.h>
   
   // Create an Indexer and connect it to a fingerprint
   // database and audio provider
   std::unique_ptr<Audioneex::Indexer>
   indexer( Audioneex::Indexer::Create() );
   indexer->SetDataStore( someDatastore );
   indexer->SetAudioProvider( someAudioProvider );
   
   // Start the indexing session
   indexer->Start()
   
   // For each audio recording, call the Index() method by
   // passing a progressive (strictly increasing) fingerprint id.
   // This will block and cause the engine to repeatedly call the 
   // OnAudioData() handler of the connected audio provider until 
   // all audio data for the current recording is exhausted. 
   while ( haveRecordingsToIndex )
   {
       // ...
       
       indexer->Index( recID++ );
   }
   
   // End the indexing session
   indexer->End()

The audio provider is any class implementing the :class:`AudioProvider` interface 
(a "listener"). Then in the ``OnAudioData()`` handler just feed the requested 
amount of audio to the engine (indicated by the ``nsamples`` parameter) in the 
internal buffer until all audio is consumed

.. code-block:: cpp

   int SomeAudioProvider::OnAudioData(uint32_t FID, 
                                      float *buffer, 
                                      size_t nsamples)
   {
       // Read nsamples from the source in 16 bit, 11025Hz, mono
       auto audio = someAudioSource->Read( nsamples, 16, 11025, 1 );
    
       if( audio )
           copy( audio->data(), buffer, nsamples )
        
       return audio->size();
   }

Obviously this is a simplified code, but that's the whole process. To perform 
identifications, create an instance of :class:`Recognizer`, set the relevant 
properties and feed it with audio, as follows

.. code-block:: cpp

   // Create a Recognizer and connect it to a fingerprint database
   std::unique_ptr<Audioneex::Recognizer>
   recognizer( Audioneex::Recognizer::Create() );
   recognizer->SetDataStore( someDataStore );
   
   // Set other properties
   // ...
   
   // Feed audio in 1.5s-long chunks at 16 bit, 11025Hz, mono
   // until there is a response or there is no more audio
   do{
      audio = someAudioSource->Read( 1.5, 16, 11025, 1 );
      recognizer->Identify( audio->data(), audio->size() )
      results = recognizer->GetResults();
   }
   while(audio && !results);

   if( results )
       DoSomething( results );

Please refer to the example programs for more details. There are also few 
important things to be aware of. The engine uses an error handling system based 
on C++ exceptions (enabled with ``/Ehsc`` in VC++ and ``-fexceptions`` in GCC) 
and most API methods throw exceptions, which cross the library boundaries. This
means that the libraries should be used with a matching compiler version to 
avoid nasty surprises at runtime.

