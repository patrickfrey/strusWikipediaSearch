#!/bin/sh

strusRpcServer -m analyzer_wikipedia_search -s "path=/data/wikipedia/storage2" -R resources/ -p 7182 -g storage_stats2.txt
