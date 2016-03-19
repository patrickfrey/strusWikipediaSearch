#!/bin/sh

strusDumpStorage -P r -s path=storage | scripts/getRandomQueries.pl 200 BM25pff

