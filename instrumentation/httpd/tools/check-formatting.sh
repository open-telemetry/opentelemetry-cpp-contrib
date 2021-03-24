#!/bin/bash

ARGS="${1:-code build}"

SELF=`dirname "$0"`

print_help () {
   echo "Checking code formatting"
   echo "$0 [code] [build]"
}

check_clang () {
   ${SELF} ./format-code.sh
}

check_buildifier () {
  ${SELF} ./format-bazel.sh
}

check_all () {
  while [[ $# > 0 ]] ; do case "$1" in
    code|clang)
      check_clang
      shift
      ;;
    build|buildifier)
      check_buildifier
      shift
      ;;
    -h)
      print_help
      exit 0
      ;;
    *)
      print_help
      echo "Unknown argument: $1" >&2
      exit 1
    ;;
  esac done
}

check_all ${ARGS}
