#!/bin/bash

query()
{
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=world+is+not+enough&n=20&scheme=BM25_dpfc&x=0&y=0
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=supply+room&n=20&scheme=BM25_dpfc&x=0&y=0
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=english+breakfast&n=20&scheme=BM25_dpfc&x=0&y=0
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=dinner+for+two&n=20&scheme=BM25_dpfc&x=0&y=0
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=sex+drugs+rock&n=20&scheme=BM25_dpfc&x=0&y=0
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=psychodelic+rock&n=20&scheme=BM25_dpfc&x=0&y=0
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=garage+rock&n=20&scheme=BM25_dpfc&x=26&y=24
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=pink+floyd&n=20&scheme=BM25_dpfc&x=0&y=0
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=alice+cooper&n=20&scheme=BM25_dpfc&x=0&y=0
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=frankenstein&n=20&scheme=BM25_dpfc&x=0&y=0
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=+pistepirkko&n=20&scheme=BM25_dpfc&x=0&y=0
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=+shakes+beer&n=20&scheme=BM25_dpfc&x=0&y=0
}
export -f query

dowork()
{
	for ii in 1 2 3 4 5; do query; done
}
export -f dowork

sem -j 20 dowork

