#!/bin/bash

# directory one level up from this script directory
PROJECT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )"/.. && pwd )

if ! ${BUILDIFIER:=buildifier} -version ; then
   echo ${BUILDIFIER} not found in '$PATH' = $PATH >&2
   exit 1
fi


if [ "$1" == "-i" ]; then # apply
  MODE="-mode fix -v"
  shift
else
  MODE="-mode diff"
fi

if [[ "$#" -gt 0 && -d "$1" ]]; then
  PROJECT_DIR="$1"
  shift
fi

SRC_FILES=${1:-$(find "$PROJECT_DIR"  -path '*/.*' -prune -name WORKSPACE -print -o -name BUILD -print -o \
    -name '*.BUILD' -o -name '*.bzl' -print)}

${BUILDIFIER} ${MODE} ${SRC_FILES}

BADFORMAT=$?
if [ "$BADFORMAT" != "0" ]; then
   echo >&2
   echo "There are some files with broken formating, please run following command to fix all of them:" >&2
   echo "$0 -i" >&2
   exit 1
fi
