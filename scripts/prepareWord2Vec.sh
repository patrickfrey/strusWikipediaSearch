#!/bin/sh

strusInspect -s "path=storage1" content orig | perl -pe 's/^[0-9]+[:]//' > docs.1.txt
strusInspect -s "path=storage2" content orig | perl -pe 's/^[0-9]+[:]//' > docs.2.txt
strusInspect -s "path=storage3" content orig | perl -pe 's/^[0-9]+[:]//' > docs.3.txt

cat docs.{1,2,3}.txt | iconv -c -f utf-8 -t utf-8 - | perl -pe 's/\&[a-z]*\;/ /g' | perl -pe 's/nbsp[\;]/ /g' | perl -pe 's/[––_=\/;,()\[\]\{\}\"-\-]/ /g' | perl -pe 's/([\!\?\:\.])/ \1 /g' | perl -pe 's/[ ]+/ /g' | perl -pe 's/[ ][.][ ]/ .\n/g'  | scripts/strusnlp.py | less
../word2vec/bin/word2vec -size 300 -window 5 -sample 1e-5 -negative 8 -threads 4 -min-count 4 -alpha 0.025 -classes 0 -debug 1 -binary 1 -save-vocab vocab.txt -cbow 0 -train docs.txt -output vectors.bin



