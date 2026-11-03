#!/bin/sh
set -e

tools/buildvendor.sh
premake5 gmake $@
make clean config=debug
make clean config=release
make config=release
tools/postbuild.sh release
