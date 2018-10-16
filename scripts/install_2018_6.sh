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

for aa in 0 1 2 3 4 5 6 7 8 9; do
for bb in 0 1 2 3 4 5 6 7 8 9; do
for cc in 0 1 2 3 4 5 6 7 8 9; do
for dd in 0 1 2 3 4 5 6 7 8 9; do
	DID=$aa$bb$cc$dd
	strusPosTagger -I -C XML -e '//pagelink()' -e '//weblink()' -e '//text()' -e '//attr()' -e '//char()' -e '//math()' -e '//code()' -e '//bibref()' -E '//attr' -E '//attr~' -p '//heading' -p '//table' -p '//citation' -p '//ref' -p '//list' -p '//quot' -p '//cell~' -p '//head~' -p '//heading~' -p '//list~' -D '; ' /srv/wikipedia/doc/$DID /srv/wikipedia/pos/$DID.txt
done
done
done
done


PARSER_EVAL=bazel-bin/syntaxnet/parser_eval
MODEL_DIR=syntaxnet/models/parsey_mcparseface
cat /srv/wikipedia/pos/0000.txt | sed 's/;/;\n/g' | sed 's/[.]/.\n/g' | $PARSER_EVAL --input stdin --output stdout-conll --model_path $MODEL_DIR/tagger-params --task_context $MODEL_DIR/context.pbtxt --hidden_layer_sizes 64 --arg_prefix brain_tagger --graph_builder structured --slim_model --batch_size 32 | $PARSER_EVAL --input stdin-conll --output stdout-conll --hidden_layer_sizes 512,512 --arg_prefix brain_parser --graph_builder structured --task_context $MODEL_DIR/context.pbtxt --model_path $MODEL_DIR/parser-params --slim_model --batch_size 32
