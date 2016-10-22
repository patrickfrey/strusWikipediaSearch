#!/bin/sh

outprefix = nlpdata/

runNLP() {
	jobid=$1
	infiles=$2
	dmp_outputfile = "$outprefix""docs.dump.$jobid.txt"
	nlp_outputfile = "$outprefix""docs.nlp.$jobid.txt"
	dic_outputfile = "$outprefix""dict.$jobid.txt"
	rm $dmp_outputfile
	rm $nlp_outputfile
	for dd in $infiles ; do echo "-------- $dd"; github/strusWikipediaSearch/scripts/nlpdump.sh data/wikipedia$dd.tar.gz $jobid "$outprefix"tmp $dmp_outputfile; done
	github/strusWikipediaSearch/scripts/strusnlp.py nlp $dmp_outputfile > $nlp_outputfile
	rm $dmp_outputfile
	github/strusWikipediaSearch/scripts/strusnlp.py dict $nlp_outputfile 3 > $dic_outputfile
}

runNLP 1 "00 13 03 16" &
runNLP 2 "06 19 09 22" &
runNLP 3 "01 14 04 17" &
runNLP 4 "07 20 10 23 25" &
runNLP 5 "02 15 05 18 12" &
runNLP 6 "08 21 11 24 26" &

scripts/strusnlp.py joindict dict.{1,2,3,4,5,6}.txt > "$outprefix"dict.txt
rm dict.{1,2,3,4,5,6}.txt

for dd in 1 2 3 4 5 6
do
scripts/strusnlp.py concat "$outprefix"docs.nlp.$jobid.txt "$outprefix"dict.txt > "$outprefix"docs.word2vec.$dd.txt
done

cat "$outprefix"docs.word2vec.{1,2,3,4,5,6}.txt > "$outprefix"docs.word2vec.txt

../word2vec/bin/word2vec -size 300 -window 8 -sample 1e-5 -negative 8 -threads 4 -min-count 4 -alpha 0.025 -classes 0 -debug 1 -binary 1 -save-vocab vocab.txt -cbow 0 -train "$outprefix"docs.word2vec.txt -output vectors.bin


