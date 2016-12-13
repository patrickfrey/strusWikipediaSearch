#!/bin/sh

strusCreate -S config/storage.conf
for dd in 00 03 06 09 12 15 18 21 24 ; do echo "-------- $dd"; scripts/insert.sh data/wikipedia$dd.tar.gz; done
mv storage storage1
strusCreate -S config/storage.conf
for dd in 01 04 07 10 13 16 19 22 25; do echo "-------- $dd"; scripts/insert.sh data/wikipedia$dd.tar.gz; done
mv storage storage2
strusCreate -S config/storage.conf
for dd in 02 05 08 11 14 17 20 23 26; do echo "-------- $dd"; scripts/insert.sh data/wikipedia$dd.tar.gz; done
mv storage storage3

