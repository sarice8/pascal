#!/bin/sh

./bin-${OSTYPE}/pascal -l $*

./bin-${OSTYPE}/sm a.out
