#!/bin/bash

echo "Compiling"
gcc bar.c -o xbar -Wall -lX11

echo "Moving to /usr/bin"
sudo cp xbar /usr/bin
