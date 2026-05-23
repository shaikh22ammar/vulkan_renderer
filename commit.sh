#!/bin/bash
if [ $# -lt 1 ]; then
	echo "Usage: $0 \"commit message\""
	exit 1
fi
git add src third_party
git commit -m "$1"
