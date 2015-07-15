#!/bin/bash

dowork()
{
	for kk in 1 2 3 4 5 6 7 8 9 0
	do
		for ii in 1 2 3 4 5 6 7 8 9 0; do ./testQuery.sh; done
	done
}
export -f dowork

sem -j 20 dowork

