#!/bin/bash
#
#  Audioneex dependencies installer
#  --------------------------------
#
#  This script downloads, builds and installs the required dependencies 
#  from the sources (TokyoCabinet is actually available as a PPA and can
#  be installed by apt but we'll include it here anyways). It also edits
#  the main CMake script to add the dependencies include and lib paths to
#  the build.
#
#  NOTE: This script MUST be executed from the source root directory.
#        It is currently only used on Continuous Integration platforms
#        (actually Travis CI) to install the required dependencies, but
#        it may as well be integrated in the project's build process to 
#        make it fully automated.
#

# Installation directories
INSTALLDIR="$(pwd)/_deps"
IPREFIX="--prefix=${INSTALLDIR}"
INST_H_DIR="--includedir=${INSTALLDIR}/include"
INST_LIB_DIR="--libdir=${INSTALLDIR}/lib/x64"

# Configuration lines to be added to the CMake script for the
# dependencies' include and library paths.
SLINE1="set(MY_INCLUDE_DIRS"
ILINE1="    \${CMAKE_SOURCE_DIR}\/_deps\/include\n"
ILINE1+="    \${CMAKE_SOURCE_DIR}\/_deps\/include\/tcabinet\n"
ILINE1+="    \${CMAKE_SOURCE_DIR}\/_deps\/include\/fftss"
SLINE2="set(MY_LIBRARY_x64_RELEASE_DIRS"
ILINE2="    \${CMAKE_SOURCE_DIR}\/_deps\/lib\/x64"
IFILE="../CMakeLists.txt"

# Configuration parameters for each dependency
FFTSS_CONFIG_PARAMS="\
  CFLAGS=-fPIC \
  ${IPREFIX} \
  ${INST_H_DIR}/fftss \
  ${INST_LIB_DIR}"

TOKYO_CONFIG_PARAMS="\
  --disable-zlib \
  --disable-bzip \
  ${IPREFIX} \
  ${INST_H_DIR}/tcabinet \
  ${INST_LIB_DIR}"

# The packages' URLs
DEPS=(
   "https://www.ssisc.org/fftss/dl/fftss-3.0-20071031.tar.gz"
   "https://fallabs.com/tokyocabinet/tokyocabinet-1.4.48.tar.gz"
)

# Configuration parameters map
declare -A DEPS_CONFIG_PARAMS=(
   ["fftss-3.0-20071031"]="$FFTSS_CONFIG_PARAMS"
   ["tokyocabinet-1.4.48"]="$TOKYO_CONFIG_PARAMS"
)


if [ -d "_deps" ]; then
   echo ""
   echo "Looks like the dependencies are already installed."
   echo "If you want to re-install them, remove the '_deps'"
   echo "directory from the root."
   echo""
   exit 0
fi

mkdir _deps && cd _deps

# Download, make and install the dependencies
for DEP in "${DEPS[@]}"; do
    DEPDIR=$(basename $DEP .tar.gz)
    wget $DEP
    tar -xzvf ${DEPDIR}.tar.gz
    cd ${DEPDIR}
    ./configure ${DEPS_CONFIG_PARAMS[$DEPDIR]}
    make && make install
    cd ..
    rm -rf ${DEPDIR}
    rm -f ${DEPDIR}.tar.gz
done

# Edit the CMake main script
sed -i "s/$SLINE1/$SLINE1\n$ILINE1/" $IFILE
sed -i "s/$SLINE2/$SLINE2\n$ILINE2/" $IFILE

cd ..

