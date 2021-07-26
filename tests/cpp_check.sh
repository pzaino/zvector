#!/bin/bash

IS_CPPCHECK=$(which cppcheck | grep -Poi "no cppcheck in" | wc -l )
if [ $IS_CPPCHECK -gt 0 ];
then
    echo "I can't find cppcheck from the shell, so have to skip this test, sorry!"
    exit 0
fi

cppcheck_cmd="$(which cppcheck)"

# Detect Platform:
Arch="$(uname -m)"
Platform=""
[ "$Arch" == "x86_64" ] && Platform="unix64"
[ "$Arch" == "x86" ] && Platform="unix32"

# Detect GCC
IS_GCC="$(which gcc 2>&1 | grep -Poi "no gcc in" | wc -l)"
if [ "$IS_GCC" == "1" ];
then
    echo "GCC not found, skipping CPPCheck analysis using GCC configuration..."
    exit 0
fi

gcc_cmd="$(which gcc)"

grep_cmd="$(which grep)"

# Detect GCC Version and library:
GCC_VER="$(${gcc_cmd} -v 2>&1 | ${grep_cmd} -Poi 'gcc version \K[0-9]+\.[0-9]+\.[0-9]+')"
GCC_TARGET="$(${gcc_cmd} -v 2>&1 | ${grep_cmd} -Poi 'Target: \K.*' )"
GCC_PATH="/usr/lib/gcc/${GCC_TARGET}/${GCC_VER}/include/"

# Detect path from where we are launching this script:
cpath=$(pwd)
bdir="$(basename "$cpath")"
dpath="$(dirname "$cpath")"

if [ "$bdir" == "zvector" ];
then
	start_path='./'
else
	start_path="$(dirname "$dpath")"
fi

# Run CPPCheck:
${cppcheck_cmd} ${start_path}/src --bug-hunting \
             --enable=all \
             --platform=${Platform} \
             --std=c99 \
             --force \
             -I/usr/include/ \
             -I/usr/include/sys/ \
             -I/usr/include/linux/ \
             -I/usr/include/gnu/ \
             -I/usr/include/bits/ \
             -I${GCC_PATH} \
             -I../ -I${start_path}/src/ \
             -io/ -ilib/ \
             --suppress=missingIncludeSystem 2>&1

exit $?
