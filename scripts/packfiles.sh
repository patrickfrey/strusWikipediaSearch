#!/bin/bash

for dd in 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30; do
    fnam="wikipedia$dd"'99.xml'
    while [ ! -f  "$fnam" ]
        do echo waiting for $fnam
        sleep 3
    done
    sleep 5 
    tar --remove-files -cvzf ./wikipedia$dd.tar.gz ./wikipedia$dd*.xml
done

