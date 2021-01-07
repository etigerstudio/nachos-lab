#!/bin/sh
# use -Q to disable verbose machine messages
echo "=== format the DISK ==="
./nachos -Q -f
echo "=== cp the compiled shell ==="
./nachos -Q -cp ../test/shell shell
