#!/usr/bin/env python3
import sys
import os


if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} <output file>")
    exit(-1)


for root, subdirs, files in os.walk("edk2/Include"):
    for file in files:
        print(file)
