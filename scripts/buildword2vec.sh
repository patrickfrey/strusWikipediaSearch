#!/bin/sh

runNLP() {
	jobid=$1
	infiles=$2
	rm docs.dump.$jobid.txt
	for dd in $infiles ; do echo "-------- $dd"; scripts/nlpdump.sh data/wikipedia$dd.tar.gz $jobid; done
	scripts/strusnlp.py nlp docs.dump.$jobid.txt > docs.nlp.$jobid.txt
	rm docs.dump.$jobid.txt
	scripts/strusnlp.py dict docs.nlp.$jobid.txt 30 > dict.$jobid.txt
}
runNLP 1 "00 13"
runNLP 2 "01 14"
runNLP 3 "02 15"
runNLP 4 "03 16"
runNLP 5 "04 17"
runNLP 6 "05 18"
runNLP 7 "06 19"
runNLP 8 "07 20"
runNLP 9 "08 21"
runNLP 10 "09 22"
runNLP 11 "10 23"
runNLP 12 "11 24"
runNLP 13 "12 25 26"

scripts/strusnlp.py joindict dict.{1,2,3,4,5,6,7,8,9,10,11,12,13}.txt > dict.txt
rm dict.{1,2,3,4,5,6,7,8,9,10,11,12,13}.txt

for dd in 1 2 3
do
scripts/strusnlp.py concat docs.nlp.$jobid.txt dict.txt > docs.word2vec.$dd.txt
done

cat docs.word2vec.{1,2,3}.txt > docs.word2vec.txt

../word2vec/bin/word2vec -size 300 -window 8 -sample 1e-5 -negative 8 -threads 4 -min-count 4 -alpha 0.025 -classes 0 -debug 1 -binary 1 -save-vocab vocab.txt -cbow 0 -train docs.word2vec.txt -output vectors.bin


