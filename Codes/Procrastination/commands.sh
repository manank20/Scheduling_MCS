#!/bin/sh
make all
./test input.txt output.txt
make clean
echo "\nOpen output.txt to see the output\n"