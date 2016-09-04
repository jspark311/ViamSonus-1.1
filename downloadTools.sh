#!/bin/bash
#
# This script is meant to go fetch the most recent versions of various tools
#   that are needed to build and upload the firmware.
# If you already have the Teesyduino environment, or some equivilent, you
#   probably don't need these things. Just alter the Makefile to point to
#   your preferred tools.

# Make the tools directory...
mkdir tools

# Teensy loader...
git clone https://github.com/PaulStoffregen/teensy_loader_cli.git tools/teensy_loader_cli

# Build it...
cd tools/teensy_loader_cli
make

# Return...
cd ..
