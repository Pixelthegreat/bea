#!/bin/sh
set -e

cd vendor/tomlc17
make
cd ../..

cd vendor/reproc
mkdir -pv build
cd build
cmake ..
make
cd ../../..
