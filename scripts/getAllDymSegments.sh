#!/bin/sh

rm data/dymitems.txt
for dd in 01; do echo "-------- $dd"; scripts/getDymSegments.sh data/wikipedia$dd.tar.gz; done

