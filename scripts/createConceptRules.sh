#!/bin/sh

outprefix=origdata/
srcprefix=github/strusWikipediaSearch/

rulefile="$outprefix""rules.txt"
dymfile="$outprefix""dymitems.txt"
dymdocs="$outprefix""dymitems.xml"

strusInspectVsm -S "$srcprefix"/vsm.conf featname > $dymfile | awk '{print $2}' | sed 's/_/ /g' > $dymfile
echo '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>' > $dymdocs
echo '<list>' >> $dymdocs
cat data/dymitems.txt | awk '{print "<item>" $0 "</item>"}' >> $dymdocs
echo '</list>' >> $dymdocs

strusInspectVsm -S "$srcprefix"/vsm.conf confeatname > $rulefile


