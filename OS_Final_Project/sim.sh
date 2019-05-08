#!/bin/bash

correctParams=true
re='^[0-9]+$'

if echo $1 | egrep -q '^[0-9]+$'; then
	echo 'Number'
    # $1 is a number
else
    # $1 is not a number
	echo 'Not'
	correctParams=false
fi

if echo $2 | egrep -q '^[0-9]+$'; then
	if ! [[ $correctParams == false ]]; then
		echo 'Number'
		./Monitor
	else
		# $2 is a number
		echo 'Failed to match with first'
	fi
else
    # $2 is not a number
	echo 'Not'
fi

echo 'Hello, World!'