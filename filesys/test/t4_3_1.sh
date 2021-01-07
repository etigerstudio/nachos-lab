#!/bin/sh
# cd /opt/module/nachos_dianti/nachos-3.4/code/filesys
echo "=== format the DISK ==="
./nachos -Q -f
echo "=== copies file \"largeFile\" from UNIX to Nachos ==="
./nachos -Q -cp test/PI1.TXT PI1
echo "=== prints the contents of the entire file system ==="
./nachos -Q -D

echo "=== remove the file \"largeFile\" from Nachos ==="
./nachos -Q -r PI1

echo "=== prints the contents of the entire file system again ==="
./nachos -Q -D