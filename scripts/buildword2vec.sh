#!/bin/sh

runNLP() {
	jobid=$1
	infiles=$2
	rm nlpdata/docs.dump.$jobid.txt
	for dd in $infiles ; do echo "-------- $dd"; github/strusWikipediaSearch/scripts/nlpdump.sh data/wikipedia$dd.tar.gz $jobid 'nlpdata/tmp'; done
	github/strusWikipediaSearch/scripts/strusnlp.py nlp nlpdata/docs.dump.$jobid.txt > nlpdata/docs.nlp.$jobid.txt
	rm nlpdata/docs.dump.$jobid.txt
	github/strusWikipediaSearch/scripts/strusnlp.py dict nlpdata/docs.nlp.$jobid.txt 3 > dict.$jobid.txt
}

runNLP 1 "00 13 03 16" &
runNLP 2 "06 19 09 22" &
runNLP 3 "01 14 04 17" &
runNLP 4 "07 20 10 23 25" &
runNLP 5 "02 15 05 18 12" &
runNLP 6 "08 21 11 24 26" &

scripts/strusnlp.py joindict dict.{1,2,3,4,5,6,7,8,9,10,11,12,13}.txt > dict.txt
rm dict.{1,2,3,4,5,6,7,8,9,10,11,12,13}.txt

for dd in 1 2 3
do
scripts/strusnlp.py concat docs.nlp.$jobid.txt dict.txt > docs.word2vec.$dd.txt
done

cat docs.word2vec.{1,2,3}.txt > docs.word2vec.txt

../word2vec/bin/word2vec -size 300 -window 8 -sample 1e-5 -negative 8 -threads 4 -min-count 4 -alpha 0.025 -classes 0 -debug 1 -binary 1 -save-vocab vocab.txt -cbow 0 -train docs.word2vec.txt -output vectors.bin


