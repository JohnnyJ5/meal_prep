#!/bin/bash

# Ensure we are always executing from the directory where this script lives (the project root)
cd "$(dirname "$(readlink -f "$0")")"

# Pass through to the new Docker-native Makefile
make stop
