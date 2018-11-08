#!/bin/sh
# Prerequisites:
#	Strus projects installed:
#		strusBase
#		strus strusAnalyzer
#		strusTrace strusModule strusRpc
#		strusPattern strusVector
#		strusUtilities strusBindings
#		strusWebService strusWikipediaSearch
#	SyntaxNet:
#		cuda 9.0 (https://yangcha.github.io/CUDA90)
#	Tensorflow (for SyntaxNet):
#		https://github.com/tensorflow/tensorflow
#	Bazel (for Tensorflow):
#		https://docs.bazel.build/versions/master/install-ubuntu.html
#	Python:
#		numpy

USER=patrick:patrick

sudo mkdir /srv/wikipedia
sudo chown $USER /srv/wikipedia
sudo mkdir /srv/wikipedia/

cd /srv/wikipedia/
wget http://dumps.wikimedia.your.org/enwiki/latest/enwiki-latest-pages-articles.xml.bz2
bunzip2 enwiki-latest-pages-articles.xml.bz2

mkdir -p xml
strusWikimediaToXml -n 0 -P 10000 -R ./redirects.txt enwiki-latest-pages-articles.xml
strusWikimediaToXml -I -B -n 0 -P 10000 -t 12 -L ./redirects.txt enwiki-latest-pages-articles.xml xml

for ext in err mis wtf org; do find xml -name "*.$ext" | xargs rm; done

POSTAGGER_TAG_OPT="-e '//pagelink()' -e '//weblink()' -e '//text()' -e '//attr()' -e '//char()' -e '//math()' -e '//code()' -e '//bibref()' -E '//entity' -E '//attr' -E '//attr~' -E '//quot' -E '//quot~' -p '//heading' -p '//table' -p '//citation' -p '//ref' -p '//list' -p '//cell~' -p '//head~' -p '//heading~' -p '//list~' -p '//br' -D '; '"

for aa in 0 1 2 3 4 5 6 7 8 9; do
for bb in 0 1 2 3 4 5 6 7 8 9; do
for cc in 0 1 2 3 4 5 6 7 8 9; do
for dd in 0 1 2 3 4 5 6 7 8 9; do
	DID=$aa$bb$cc$dd
	strusPosTagger -I -x xml -C XML $POSTAGGER_TAG_OPT /srv/wikipedia/xml/$DID /srv/wikipedia/pos/$DID.txt
done
done
done
done


cat /srv/wikipedia/pos/0000.txt | scripts/strusnlp_tagger.py -C 1 > /srv/wikipedia/tag/0000.txt
DID=0000
strusPosTagger -o odir -I -x xml -C XML $POSTAGGER_TAG_OPT /srv/wikipedia/doc/$DID /srv/wikipedia/pos/$DID.txt

strusPosTagger -o odir -I -x xml -C XML $POSTAGGER_TAG_OPT Analysis.xml t0000.txt

