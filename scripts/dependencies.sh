#!/bin/bash
#
#  Audioneex dependency installer
#  ------------------------------
#
#  This script downlods and installs dependencies that are not available
#  in public repositories (or that i wasn't able to find).
#

SLINE1="set(MY_INCLUDE_DIRS"
ILINE1="    \${CMAKE_SOURCE_DIR}\/_deps\/include"
SLINE2="set(MY_LIBRARY_x64_RELEASE_DIRS"
ILINE2="    \${CMAKE_SOURCE_DIR}\/_deps\/lib\/x64"
IFILE="../CMakeLists.txt"
DEPS_URL="https://www.dropbox.com/s/factka0h6x5tj7r/audioneex_deps.tar.gz?dl=0"

if [ -d "_deps" ]; then rm -rf _deps; fi

mkdir _deps && cd _deps
wget -O ax_deps.tar.gz $DEPS_URL
tar xf ax_deps.tar.gz


sed -i "s/$SLINE1/$SLINE1\n$ILINE1/" $IFILE
sed -i "s/$SLINE2/$SLINE2\n$ILINE2/" $IFILE

cd ..

