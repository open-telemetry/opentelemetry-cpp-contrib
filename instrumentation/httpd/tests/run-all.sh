#!/bin/bash

ERR=0

for testfile in ??-*.sh; do
   ./${testfile} || ERR=1
done

exit $ERR
