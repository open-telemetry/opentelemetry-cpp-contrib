#!/bin/bash
# specifically don't use set -e to run all tests

ERR=0

for testfile in ??-*.sh; do
   ./${testfile} || ERR=1
done

exit $ERR
