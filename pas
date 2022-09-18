#!/bin/sh

./bin-${OSTYPE}/pascal $1

# First, just dump the table without executing
./bin-${OSTYPE}/sm -d a.out

# Now execute
./bin-${OSTYPE}/sm a.out | tee a.log
