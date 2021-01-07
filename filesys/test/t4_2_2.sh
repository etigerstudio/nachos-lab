#!/bin/sh

echo "=== copies file \"small\" from UNIX to Nachos (and add extension) ==="
./nachos -Q -cp test/small I_am_a_long_long_long_long_long_long_filename.txt
sleep 1 # to observe the modification time change
echo "=== prints the contents of the entire file system ==="
./nachos -Q -D