#!/bin/sh
target="$1"
if [ "$target" = "" ]; then
	target="debug"
fi

bin/bea build stdlib -T "$target"
