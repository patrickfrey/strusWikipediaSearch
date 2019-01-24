#!/usr/bin/python3
#
# This scripts reads a analyzer dump for word2vec stdin and writes a reduced dump containing only sentences
#
from pprint import pprint
import sys
import math
import re
import time
import getopt
import os
from recordtype import recordtype
from copy import deepcopy

def isSentence( toklist, verbose):
    t_cnt = 0
    v_cnt = 0
    e_cnt = len(toklist)
    t_seq = 0
    max_t_seq = -1
    for tok in toklist:
        if not tok:
            continue
        elif tok in [",","."]:
            if max_t_seq < t_seq:
                max_t_seq = t_seq
            t_seq = 0
        elif tok[:2] == "V#":
            v_cnt += 1
            if max_t_seq < t_seq:
                max_t_seq = t_seq
            t_seq = 0
        elif tok[:2] == "T#":
            t_seq += 1
            t_cnt += 1
        else:
            if max_t_seq < t_seq:
                max_t_seq = t_seq
            t_seq = 0
    if v_cnt == 0:
        if verbose:
            print( "* reject: No verbs")
        return False
    if max_t_seq < t_seq:
        max_t_seq = t_seq
    if e_cnt > 10 and e_cnt <= 20 and t_cnt * 3 > e_cnt * 2:
        if verbose:
            print( "* reject: Untagged count: %d:%d" % (t_cnt,e_cnt))
        return False
    if e_cnt > 20 and t_cnt * 2 > e_cnt:
        if verbose:
            print( "* reject: Untagged count: %d:%d" % (t_cnt,e_cnt))
        return False
    if max_t_seq > 5:
        if verbose:
            print( "* reject: Max sequence %d" % (max_t_seq))
        return False
    return True

def run( verbose):
    tokbuf = []
    heading = ""
    for line in sys.stdin:
        tokbuf.extend( line.strip(" \t\n").split(' '))
        while "." in tokbuf:
            idx = tokbuf.index(".")
            sequence = tokbuf[:idx]
            leadtok = heading
            for tok in sequence:
                if tok[:2] == "H#":
                    heading = tok
                    leadtok = ""
            tokbuf = tokbuf[(idx+1):]
            if isSentence( sequence, verbose):
                if leadtok:
                    print( "%s %s ." % (leadtok, ' '.join( sequence)))
                else:
                    print( "%s ." % ' '.join( sequence))

verbose = "-V" in sys.argv
run( verbose)



