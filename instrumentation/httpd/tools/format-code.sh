#!/bin/bash

# directory one level up from this script directory
PROJECT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )"/.. && pwd )

APPLY=""

if [ "$1" == "-i" ]; then # apply
  APPLY="-i"
  shift
fi

if [[ "$#" -gt 0 && -d "$1" ]]; then
  PROJECT_DIR="$1"
  shift
fi

SRC_FILES=${1:-$(find "$PROJECT_DIR" -path '.*' -prune -name '*.c*' -o -name '*.h')}

if ! ${CLANG_FORMAT:=clang-format} --version ; then
   echo ${CLANG_FORMAT} not found in '$PATH' = $PATH >&2
   exit 1
fi

if [ "$APPLY" == "-i" ]; then
  ${CLANG_FORMAT} --verbose --style=file -i ${SRC_FILES}
  exit $?
fi

# show formatting problems
EXITCODE=0
for FILE in $SRC_FILES; do
  ${CLANG_FORMAT} --style=file "${FILE}" | git diff --no-index -- "${FILE}" -
  BADFORMAT=$?
  if [ "$BADFORMAT" != "0" ]; then
     EXITCODE=1
  fi
done

if [ "$EXITCODE" != "0" ]; then
   echo >&2
   echo "There are some files with broken formating, please run following command to fix all of them:" >&2
   echo "$0 -i" >&2
fi

exit ${EXITCODE}
