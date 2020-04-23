import os
from distutils.core import setup, Extension

INCLUDES = os.environ["INCLUDES"]
SYSLIBSDIR = os.path.join(os.environ["LIBRARIES"], "vc191", "x64", "release")

#print("LIBSDIR={}".format(LIBSDIR))

audioneex_module = Extension(
    name="_audioneex",
    sources=["audioneex.i",
             "audioneex_wrap.cpp", 
             "audioneex-py.cpp",
             "../../src/dbdrivers/TCDataStore.cpp",
             "../../src/audio/AudioSource.cpp"],
    include_dirs=["../../src", 
                  "../../src/dbdrivers",
                  "../../src/audio",
                  "../../src/tools",
                  "c:/dev/cpp/include",
                  "c:/dev/cpp/boost_1_66_0"],
    library_dirs=[SYSLIBSDIR,
                  "c:/dev/cpp/boost_1_66_0/stage/lib",
                  "../../lib\windows-x64-msvc1913/release"],
    libraries=["audioneex",
               "libejdb",
               "libboost_thread-vc141-mt-x64-1_66",
               "libboost_system-vc141-mt-x64-1_66"],
    define_macros=[("WIN32","1"),
                   ("NOMINMAX","1"),
                   ("BOOST_ALL_NO_LIB","1")],
    undef_macros=[],
    extra_compile_args=[],
    swig_opts=["-c++","-outdir","./audioneex"],
    language="c++"
)

setup (name = "audioneex",
       version = "1.4.0",
       author      = "Alberto Gramaglia",
       description = "Audio recognition engine",
       ext_modules = [audioneex_module],
       py_modules = ["audioneex"],
       )
