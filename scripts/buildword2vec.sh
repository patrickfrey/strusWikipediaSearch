#!/bin/sh

jobid=1
rm docs.dump.$jobid.txt
for dd in 00 03 06 09 12 15 18 21 24 ; do echo "-------- $dd"; scripts/nlp.sh data/wikipedia$dd.tar.gz $jobid; done
time scripts/strusnlp.py nlp docs.dump.$jobid.txt > docs.nlp.$jobid.txt
time scripts/strusnlp.py dict docs.nlp.$jobid.txt 30 > dict.$jobid.txt

jobid=2
rm docs.dump.$jobid.txt
for dd in 01 04 07 10 13 16 19 22 25;  do echo "-------- $dd"; scripts/nlp.sh data/wikipedia$dd.tar.gz $jobid; done
time scripts/strusnlp.py nlp docs.dump.$jobid.txt > docs.nlp.$jobid.txt
time scripts/strusnlp.py dict docs.nlp.$jobid.txt 30 > dict.$jobid.txt

jobid=3
rm docs.dump.$jobid.txt
for dd in 02 05 08 11 14 17 20 23;     do echo "-------- $dd"; scripts/nlp.sh data/wikipedia$dd.tar.gz $jobid; done
time scripts/strusnlp.py nlp docs.dump.$jobid.txt > docs.nlp.$jobid.txt
time scripts/strusnlp.py dict docs.nlp.$jobid.txt 30 > dict.$jobid.txt

scripts/strusnlp.py joindict dict.{1,2,3}.txt > dict.txt
rm dict.{1,2,3}.txt

for dd in 1 2 3
do
scripts/strusnlp.py concat docs.nlp.$jobid.txt dict.txt > docs.word2vec.$dd.txt
done

cat docs.word2vec.{1,2,3}.txt > docs.word2vec.txt

../word2vec/bin/word2vec -size 300 -window 8 -sample 1e-5 -negative 8 -threads 4 -min-count 4 -alpha 0.025 -classes 0 -debug 1 -binary 1 -save-vocab vocab.txt -cbow 0 -train docs.word2vec.txt -output vectors.bin


