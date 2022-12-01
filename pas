#!/bin/sh

# Runs the pascal front-end, followed by jit, or sm if using -i
#
# usage:   pas  [-i]  ...other args passed to pascal...
#  -i : run interpreter (sm) rather than x86 code (jit)
#

use_sm=0
if [ "$1" = "-i" ]; then
  use_sm=1
  # Remove from $@
  shift
fi


./bin-${OSTYPE}/pascal -l $@
status=$?
if [ $status -ne 0 ]; then
  exit $status
fi

if [ $use_sm -ne 0 ]; then
  ./bin-${OSTYPE}/sm a.out
  status=$?
else
  ./bin-${OSTYPE}/jit a.out
  status=$?
fi

exit $status

