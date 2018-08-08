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
strusWikimediaToXml -n 0 -P 10000 -R ./redirects.txt enwiki-latest-pages-articles.xml xml
strusWikimediaToXml -I -B -n 0 -P 10000 -t 12 -L ./redirects.txt enwiki-latest-pages-articles.xml xml

strusPosTagger -I -e //pagelink() -e //text() -e //attr() -p //attr~:" " 
!!!!! POS TAGGER SHOULD DEFINE PUNCTUATION STRING PER EXPRESSION
!!!!! USE CONTENT ANALYSIS OF WEBSERVICE


