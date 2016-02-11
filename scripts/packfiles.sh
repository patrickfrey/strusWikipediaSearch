#!/bin/bash

for dd in 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do
	fnam="wikipedia$dd"'00.xml'
	while [ ! -f  $fnam ]; echo waiting for $fnam; do sleep 3; done
	echo tar --remove-files -cvzf ./wikipedia$dd.tar.gz ./wikipedia$dd*.xml
done

