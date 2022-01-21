#!/bin/bash

#################################################################
#
#    Name: cpp_check.sh
# Purpose: Script to check and run CPPChecks for code quality
#  Author: Paolo Fabio Zaino
#  Domain: General
# License: Copyright by Paolo Fabio Zaino, all right reserved
#          Distributed under MIT license
#
##################################################################

###############
# Init script:
###############

# Generic Types and Macros
readonly projname="zvector"
readonly cpath="/usr/bin"
readonly csbin="/usr/sbin"
readonly cfgfile='zvector_config.h'
readonly tmpfile="/tmp/${cfgfile}.tmp"

# Check OS Type
rval="$($cpath/uname -s)"
OS_TYPE=""
case "${rval}" in
     Linux*) readonly OS_TYPE="Linux"  ;;
    Darwin*) readonly OS_TYPE="macOS"  ;;
    CYGWIN*) readonly OS_TYPE="Cygwin" ;;
     MINGW*) readonly OS_TYPE="MinGw"  ;;
          *) readonly OS_TYPE="UNKNOWN:${rval}" ;;
esac

# Check if required tools are installed:
dependecies_arr=( "grep" "sed" "basename" "dirname" "pwd" "cppcheck" )
for dep_name in "${dependecies_arr[@]}"; do
    if [ "$OS_TYPE" != "macOS" ]; then
        dep_name="${dep_name//\-/\\-}"
        check_dep="$($cpath/whereis $dep_name | $cpath/tr -d \"$dep_name:\" | $cpath/tr -d '\n')"
    else
        check_dep="$($cpath/which $dep_name | $cpath/tr -d \"$dep_name\" | $cpath/tr -d '\n')"
    fi
    if [ "$check_dep" == "" ]; then
        #(printf >&2 "%b\n" "Dependecy error: '$dep_name not found!' $cmd_name requires command $dep_name installed, without it it cannot be executed.")
        (printf "%b\n" "I can't find $dep_name, cpp_check requires command $dep_name to be installed, so skipping this test, sorry.")
        exit 0
    fi
done

# Initialization
readonly cmd_name="$($cpath/basename "$0")"

# Detect path from where we are launching this script:
curpath=$(pwd)
bdir="$(basename "$curpath")"
dpath="$(dirname "$curpath")"

if [ "$bdir" == "${projname}" ];
then
	start_path='.'
else
	start_path="$(dirname "$dpath")"
fi

cppcheck_cmd="$($cpath/which cppcheck)"

if [ "${cppcheck_cmd}" == "" ];
then
    echo "I can't find cppcheck from the shell, so have to skip this test, sorry!"
    exit 0
fi

echo ""
echo "Using cppcheck release: $(${cppcheck_cmd} --version)"
echo ""

# Detect Platform:
Arch="$(uname -m)"
Platform=""
[ "$Arch" == "x86_64" ] && Platform="unix64"
[ "$Arch" == "x86" ] && Platform="unix32"
[ "$Platform" == "" ] && Platform="native"

# Detect GCC
if [ "$OS_TYPE" != "macOS" ]; then
    gpargs='-Poi'
else
    gpargs="-oi"
fi
IS_GCC="$(w$cpath/hich gcc 2>&1 | $cpath/grep ${gpargs} "no gcc in" | $cpath/wc -l)"
if [ "$IS_GCC" == "1" ];
then
    echo "GCC not found, skipping CPPCheck analysis using GCC configuration..."
    exit 0
fi

gcc_cmd="$($cpath/which gcc)"

grep_cmd="$($cpath/which grep)"

# Detect GCC Version and library:
GCC_VER="$(${gcc_cmd} -v 2>&1 | ${grep_cmd} ${gpargs} 'gcc version \K[0-9]+\.[0-9]+\.[0-9]+')"
GCC_TARGET="$(${gcc_cmd} -v 2>&1 | ${grep_cmd} ${gpargs} 'Target: \K.*' )"
GCC_PATH="/usr/lib/gcc/${GCC_TARGET}/${GCC_VER}/include/"
if [ ! -d $GCC_PATH ];
then
  if [ "$Arch" == "x86_64" ];
  then
      GCC_PATH="/usr/lib64/gcc/${GCC_TARGET}/${GCC_VER}/include/"
      if [ ! -d $GCC_PATH ];
      then
          GCC_PATH="/usr/lib64/gcc/${GCC_TARGET}"
          mjr="$(cut -d . -f 1 <<< $GCC_VER)"
          GCC_PATH="${GCC_PATH}/${mjr}"
      fi
  fi
fi

## Determine include paths:
include_paths=""
if [ "$OS_TYPE" == "Linux" ];
then
   include_paths="-I/usr/include/ -I/usr/include/sys/ -I/usr/include/linux/ -I/usr/include/gnu/ -I/usr/include/bits/ -I${GCC_PATH} "
fi

###############
# Run CPPCheck:
###############

${cppcheck_cmd} ${start_path}/src/*.c --bug-hunting \
             --enable=all \
             --output-file=./cppcheck_report.txt \
             --platform=${Platform} \
             --std=c99 \
             --force \
             ${include_paths} \
             -I../ -I${start_path}/src/ \
             -io/ -ilib/ \
             --suppress=missingIncludeSystem -v 2>&1

exit $?

