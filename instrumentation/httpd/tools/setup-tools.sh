#!/bin/bash

BUILDIFIER_VERSION=3.5.0

ARGS="${1:-code build}"

print_help () {
   echo "Installs tools for checking code formatting"
   echo "$0 [code] [build]"
}

setup_clang () {
  export DEBIAN_FRONTEND=noninteractive
  apt-get update -y
  apt-get install -qq git
  apt-get install -qq clang-format
}

setup_buildifier () {
  apt-get install -qq curl
  curl -L -o /usr/local/bin/buildifier https://github.com/bazelbuild/buildtools/releases/download/${BUILDIFIER_VERSION}/buildifier
  chmod +x /usr/local/bin/buildifier
}

setup_all () {
  while [[ $# > 0 ]] ; do case "$1" in
    code|clang)
      setup_clang
      shift
      ;;
    build|buildifier)
      setup_buildifier
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

setup_all ${ARGS}
