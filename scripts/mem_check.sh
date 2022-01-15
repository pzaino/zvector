#!/bin/bash

#################################################################
#
#    Name: mem_check.sh
# Purpose: Script to check and run Valgrind for code quality
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
dependecies_arr=( "grep" "sed" "basename" "dirname" "pwd" "valgrind" )
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

readonly test_name="$1"

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

valgrind_cmd="$($cpath/which valgrind)"

if [ "${valgrind_cmd}" == "" ];
then
    echo "I can't find valgrind from the shell, so have to skip this test, sorry!"
    exit 0
fi

echo ""
echo "Using valgrindrelease: $(${valgrind_cmd} --version)"
echo ""

# select code to test:
if ( "$test_name" != "" );
then
    code_to_check="$test_name"
else
    #code_to_check="${start_path}/tests/bin/02ITest003"
    code_to_check="${start_path}/tests/bin/01UTest001"
fi

###############
# Run Analysis:
###############

out_route="2>&1"
out_route=""

# file report?
extra_opts="--log-file=valgrind-out.txt"
extra_opts=""

${valgrind_cmd} --leak-check=full \
            --show-leak-kinds=all \
            --track-origins=yes \
            --verbose \
            ${extra_opts} ${code_to_check} ${out_route}

exit $?

