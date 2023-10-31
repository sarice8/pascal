#!/bin/sh

# Runs the pascal front-end, followed by jit, or sm if using -i
#
# usage:   pas  [-i]  ...other args passed to pascal...
#  -i : run interpreter (sm) rather than x86 code (jit)
#

ROOT=.

use_sm=0
if [ "$1" = "-i" ]; then
  use_sm=1
  # Remove from $@
  shift
fi


${ROOT}/bin-${OSTYPE}/pascal -l -I${ROOT} $@
status=$?
if [ $status -ne 0 ]; then
  exit $status
fi

if [ $use_sm -ne 0 ]; then
  ${ROOT}/bin-${OSTYPE}/sm a.out
  status=$?
else
  ${ROOT}/bin-${OSTYPE}/jit a.out
  status=$?
fi

exit $status

