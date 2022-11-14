#!/bin/sh

./bin-${OSTYPE}/pascal -l $*

./bin-${OSTYPE}/sm a.out | tee a.log
