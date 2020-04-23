
%module(directors="1") audioneex

%include <std_string.i>
%include <std_vector.i>
%include <std_shared_ptr.i>
%include <exception.i>

%{
#include "audioneex-py.h"
%}

template<class T> using AxRef = std::shared_ptr<T>;
template<class T> using AxPtr = std::unique_ptr<T>;
template<class T> using AxVec = std::vector<T>;
typedef std::string     AxStr;


// -----------------------------------------------------------------------------
// Exceptions handling wrapper. Currently, all C++ exceptions are
// mapped into Python's RuntimeError's.
// -----------------------------------------------------------------------------

%exception {
  try {
    $action
  } catch (const std::exception& e) {
    SWIG_exception(SWIG_RuntimeError, e.what());
  }
}


// -----------------------------------------------------------------------------
// Some C++ objects are shared with Python, so they are wrapped 
// into shared pointers for automatic memory management.
// -----------------------------------------------------------------------------

%shared_ptr(AxPyDatabase)
%shared_ptr(AxPyCallbacks)
%shared_ptr(AxPyAudioBuffer)
%shared_ptr(AxPyIdResults)
//%shared_ptr(AxPyIdMatch)


// -----------------------------------------------------------------------------
// Instantiate all the templates that must be wrapped.
// -----------------------------------------------------------------------------

%template(RefDatabase) AxRef<AxPyDatabase>;
%template(RefCallbacks) AxRef<AxPyCallbacks>;
%template(RefAudioBuffer) AxRef<AxPyAudioBuffer>;
%template(RefIdResults) AxRef<AxPyIdResults>;
//%template(RefIdMatch) AxRef<AxPyIdMatch>;


// -----------------------------------------------------------------------------
// Interface names management.
// -----------------------------------------------------------------------------

// Do not expose private/internal symbols starting with "_"
%rename("%(regex:/^_(.*)/$ignore/)s") "";

// Do not expose internal "bridging" callbacks
%ignore AxPyCallbacks::OnAudioData;

// Strip the "AxPy" prefix from the Python interface symbols
%rename("%(regex:/^AxPy(.*)/\\1/)s") "";


// -----------------------------------------------------------------------------
// Provide a minimal Python interface to iterate sequence-like objects.
// -----------------------------------------------------------------------------

%rename(__getitem__) AxPyAudioBuffer::get_item;
%rename(__setitem__) AxPyAudioBuffer::set_item;
%rename(__len__)     AxPyAudioBuffer::get_length;
%rename(__iter__)    AxPyAudioBuffer::iterator;

%rename(__getitem__) AxPyIdResults::get_item;
%rename(__setitem__) AxPyIdResults::set_item;
%rename(__len__)     AxPyIdResults::get_length;
%rename(__iter__)    AxPyIdResults::iterator;

%rename(__next__)    AxIterator<AxPyIdResults, AxPyIdMatch>::next;
%rename(__next__)    AxIterator<AxPyAudioBuffer, float>::next;


// -----------------------------------------------------------------------------
// Enable the "director" feature to allow calling Python functions 
// from C++ (used for audio callbacks).
// -----------------------------------------------------------------------------

%feature("director") AxPyCallbacks;


// -----------------------------------------------------------------------------
// Override the AxIterator::next() method in Python to detect the
// end of the iterator and signal it "Pythonically" (that is by 
// raising a StopIteration exception).
// -----------------------------------------------------------------------------

%feature("shadow") AxIterator<AxPyIdResults, AxPyIdMatch>::next() %{
def __next__(self):
    try:
        return $action(self)
    except Exception as e:
        if str(e) == "__AX_ITERATOR_END__":
            raise StopIteration
        else:
            raise RuntimeError(e)
%}

%feature("shadow") AxIterator<AxPyAudioBuffer, float>::next() %{
def __next__(self):
    try:
        return $action(self)
    except Exception as e:
        if str(e) == "__AX_ITERATOR_END__":
            raise StopIteration
        else:
            raise RuntimeError(e)
%}


// -----------------------------------------------------------------------------
// Wrap the iterable objects with their iterators
// -----------------------------------------------------------------------------

%pythonappend AxPyIdResults::iterator() %{
    val.set(self)
%}

%pythonappend AxPyAudioBuffer::iterator() %{
    val.set(self)
%}


// -----------------------------------------------------------------------------
// Store the audio provider object in the Python Indexer instance. 
// This is needed because if the Python Callback object is for some
// reason dereferenced and garbage-collected while the Indexer is 
// working, the internal C++ object (the director) will call a dead
// Python method, crashing the application.
// -----------------------------------------------------------------------------

%pythonappend AxPyIndexer::AxPyIndexer(AxRef<AxPyDatabase> dstore, 
                                       AxRef<AxPyCallbacks> aprovider) %{
    if(len(args) == 2):
        self._aprovider = args[1]
%}

%pythonappend AxPyIndexer::set_audio_provider(AxRef<AxPyCallbacks> aprovider) %{
    self._aprovider = aprovider
%}


// -----------------------------------------------------------------------------
// Include the interfaces to be wrapped.
// -----------------------------------------------------------------------------


%include "audioneex-py.h"


// -----------------------------------------------------------------------------
// These templates need to located here since they are only available
// after the inclusion of the main .h file, where they are defined.
// -----------------------------------------------------------------------------

%template(_IdResultsIterator) AxIterator<AxPyIdResults, AxPyIdMatch>;
%template(_AudioBufferIterator) AxIterator<AxPyAudioBuffer, float>;

