#!/bin/bash
# This script is intended to continuously send a locomotive STOP message
# every second.
# This is intended for testing pidcc.
#
# Usage (typical):
#   ./periodic.sh | /usr/local/bin/pidcc

# echo "debug"
echo "pin $1 $2"
shift
shift
while true ; do echo "send $*" ; sleep 1 ; done

