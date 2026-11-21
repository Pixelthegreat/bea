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

cd vendor/qbe
make
cd ../..

cd vendor/slkwvr
make
cd ../..

mkdir -pv vendor/bin
cp -v vendor/qbe/qbe vendor/bin
cp -v vendor/slkwvr/slkwvr vendor/bin
