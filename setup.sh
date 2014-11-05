#!/bin/bash

echo "Compiling"
gcc bar.c -o bar -Wall -lX11

echo "Moving to /usr/bin"
sudo cp bar /usr/bin
