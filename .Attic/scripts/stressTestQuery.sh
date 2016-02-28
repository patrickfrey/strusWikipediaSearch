#!/bin/bash

query()
{
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=world+is+not+enough&n=20&scheme=NBLNK
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=supply+room&n=20&scheme=NBLNK
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=english+breakfast&n=20&scheme=NBLNK
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=dinner+for+two&n=20&scheme=NBLNK
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=sex+drugs+rock&n=20&scheme=NBLNK
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=psychodelic+rock&n=20&scheme=NBLNK
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=garage+rock&n=20&scheme=NBLNK&x=26&y=24
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=pink+floyd&n=20&scheme=NBLNK
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=alice+cooper&n=20&scheme=NBLNK
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=frankenstein&n=20&scheme=NBLNK
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=+pistepirkko&n=20&scheme=NBLNK
	wget -O - http://127.0.0.1/strus/evalQuery.php?q=+shakes+beer&n=20&scheme=NBLNK
}
export -f query

dowork()
{
	for kk in 1 2 3 4 5 6 7 8 9 0
	do
		for ii in 1 2 3 4 5 6 7 8 9 0; do query; done
	done
}
export -f dowork

sem -j 20 dowork

