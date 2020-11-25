#!/bin/sh
make all
./test input.txt output.txt
make clean
clear
echo "Open output.txt to see the output"