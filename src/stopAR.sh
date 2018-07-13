#!/bin/bash

P=`ps -e | grep alleleRegistry | sed 's/^ *//g' | cut -d \  -f 1`

if [ -z "${P}" ]
then
	echo "Cannot find AR process"
	exit 1
fi

kill ${P}


