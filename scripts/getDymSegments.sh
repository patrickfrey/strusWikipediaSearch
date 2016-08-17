#!/bin/sh

mkdir -p tmp
tar -C tmp/ -xvzf $1
for ff in `ls tmp/*.xml`; do strusSegment -e '/mediawiki/page/title()' -e '/mediawiki/page/revision/text/h1()' -e '/mediawiki/page/revision/text/h2()' -e '/mediawiki/page/revision/text/h3()' -e '/mediawiki/page/revision/text/h4()' -e '/mediawiki/page/revision/text/h5()' -e '/mediawiki/page/revision/text/h6()' -e '/mediawiki/page/revision/text/h1()' -e '/mediawiki/page/revision/text//link[@type="page"]()' $ff >> data/dymitems.txt; done
rm -Rf tmp/
