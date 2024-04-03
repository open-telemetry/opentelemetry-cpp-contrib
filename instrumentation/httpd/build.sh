#!/bin/sh
set -e

bazel build -c opt :all
