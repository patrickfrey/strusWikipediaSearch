#!/bin/sh

rm data/dymitems.txt
for dd in 00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 ; do echo "-------- $dd"; scripts/getDymSegments.sh data/wikipedia$dd.tar.gz; done
sort data/dymitems.txt | uniq > data/dymitems_sorted.txt
mv data/dymitems_sorted.txt data/dymitems.txt

echo '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>' > data/dymitems.xml
echo '<list>' >> data/dymitems.xml
cat data/dymitems.txt | awk '{print "<item>" $0 "</item>"}' >> data/dymitems.xml
echo '</list>' >> data/dymitems.xml

