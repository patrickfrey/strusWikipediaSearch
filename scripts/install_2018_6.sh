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
sudo mkdir /srv/wikipedia/raw		# Wikipedia dump converted to XML

wget -q -O - http://dumps.wikimedia.your.org/enwiki/latest/enwiki-latest-pages-articles.xml.bz2 \
	| bzip2 -d -c | iconv -c -f utf-8 -t utf-8 - > /srv/wikipedia/dump.wikimedia.xml
	
strusWikimediaToXml -f "/srv/wikipedia/raw/wikipedia%04u.xml,10M" -n0 -s /srv/wikipedia/dump.wikimedia.xml



