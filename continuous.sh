#!/bin/bash
# This script is intended to continuously send a locomotive STOP message.
# This is intended for testing pidcc.
#
# Usage (typical):
#   ./continuous.sh | /usr/local/bin/pidcc

# echo "debug"
echo "pin $1 $2"
shift
shift
echo "silent"
while true ; do echo "send $*" ; done

