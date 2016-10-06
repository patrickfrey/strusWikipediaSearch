#!/usr/bin/python
import tornado.ioloop
import tornado.web
import tornado.gen
import tornado.iostream
import os
import sys
import struct
import collections
import optparse
import binascii
import time
import strusMessage
import strus
import itertools
import heapq
import re
import collections
import string
from inspect import getmembers
from pprint import pprint
from copy import copy

# Information retrieval engine:
dymBackend = None
# IO loop:
global debugtrace
debugtrace = False
# Strus client connection factory:
msgclient = strusMessage.RequestClient()

# Structure data types:
DymItem = collections.namedtuple('DymItem', ["phrase","weight"])
ItemOccupation = collections.namedtuple('ItemOccupation', ["list","weight"])

class DymBackend:
    # Create a query evaluation scheme for "did you mean" query proposals:
    def createQueryEval_dym( self):
        rt = self.context.createQueryEval()

        # Declare the feature used for selecting result candidates:
        rt.addSelectionFeature( "selfeat")

        # Query evaluation scheme:
        rt.addWeightingFunction( "td", {
                    ".match": "docfeat"
        })
        rt.addWeightingFunction( "metadata", {
                    "name": "doclen"
        })
        rt.addWeightingFormula( "0.5 * _0 + (_0 / (2 + _1)) * 0.5", {});

        # Summarizer for getting the document title:
        rt.addSummarizer( "attribute", { "name": "docid" })
        return rt

    # Constructor. Initializes the query evaluation schemes and the query and document analyzers:
    def __init__(self, config):
        # Open local storage on file with configuration specified:
        self.context = strus.Context()
        self.storage_dym = self.context.createStorageClient( config )
        self.queryeval_dym = self.createQueryEval_dym()               # dym = did you mean ... ?
        self.analyzer = self.context.createQueryAnalyzer()
        self.analyzer.definePhraseType(
                    "dym", "ngram", "word", 
                    ["lc", [ "ngram", "WithStart", 3]]
        )
        self.analyzer.definePhraseType(
                    "word", "word", "word", 
                    ["lc", [ "convdia", "en"]]
        )

    @staticmethod
    def getCardinality( featlen):
        if (featlen >= 4):
            return 3
        if (featlen >= 2):
            return 2
        return 1

    @staticmethod
    def prefixEditDist_( s1, p1, s2, p2, dist):
        l1 = len(s1)
        l2 = len(s2)

        while p1 < l1 and p2 < l2:
            if s1[ p1].lower() == s2[ p2].lower():
                p1 += 1
                p2 += 1
                pass
            elif dist == 0:
                return -1
            else:
                # Case 1 - Replace one character in string s1 with one from s2:
                rt = DymBackend.prefixEditDist_( s1, p1+1, s2, p2+1, dist-1)
                if (rt >= 0):
                    return rt + 1
                # Case 2 - Remove one character from string s1:
                rt = DymBackend.prefixEditDist_( s1, p1+1, s2, p2, dist-1)
                if (rt >= 0):
                    return rt + 1
                # Case 3 - Remove one character from string s2:
                rt = DymBackend.prefixEditDist_( s1, p1, s2, p2+1, dist-1)
                if (rt >= 0):
                    return rt + 1
                return -1
        if (p1==p2 or p2==l2) and p1==l1:
            return 0
        else:
            return -1

    @staticmethod
    def prefixEditDist( s1, s2):
        dist = 2
        if len( s1) < 4:
            dist = 1
        if len( s1) < 3:
            dist = 0
        return DymBackend.prefixEditDist_( s1, 0, s2, 0, dist)

    @staticmethod
    def getBestElemOccuppation( terms, elems):
        if not terms:
            return []
        occupation = [ ItemOccupation( [], 0.0)]
        for termidx,term in enumerate(terms):
            tmp_occupation = []
            for elemidx,elem in enumerate( elems):
                dist = DymBackend.prefixEditDist( term, elem)
                if dist < 0:
                    continue
                for oc in occupation:
                    if not elemidx in oc.list:
                        tmp_occupation.append( ItemOccupation( oc.list + [elemidx], oc.weight + 1.0/(dist+1) ))
            occupation = tmp_occupation
            if not occupation:
                return None
        maxorderdist = 9 
        rt = None
        for oc in occupation:
            orderdist = 0
            li = 1
            while li < len(oc.list):
                if oc.list[li] < oc.list[li-1]:
                    oc.list[li-1],oc.list[li] = oc.list[li],oc.list[li-1]
                    orderdist += 1
                    if orderdist > maxorderdist:
                        break
                    if li > 1:
                        li -= 1
                else:
                    li += 1
            orderdist += oc.list[0] + oc.list[-1] - len(oc.list) + 1
            if orderdist < maxorderdist:
                weight = (0.75 * oc.weight) + (0.25 * oc.weight / (orderdist+3))
                if not rt or rt.weight < weight:
                    rt = ItemOccupation( oc.list, weight)
        return rt

    # Query for retrieval of 'did you mean' proposals:
    def evaluateQuery( self, querystr, nofranks, restrictdnlist):
        # Remove common start terms in old query string
        terms = querystr.split()
        if not terms:
            return []

        # Analyze query:
        ngrams = self.analyzer.analyzePhrase( "dym", querystr)
        words  = self.analyzer.analyzePhrase( "word", querystr)
        if not words or not ngrams:
            # Return empty result for empty query:
            return []

        # Build the query:
        queryeval = self.queryeval_dym
        query = queryeval.createQuery( self.storage_dym)
        selexprlist = []

        selexpr = ["contains", 0, 0]
        position = 0
        prev_first = None
        this_first = None
        for term in ngrams:
            if (term.position() != position):
                prev_first,this_first = this_first,term
                if (position != 0):
                    selexpr[2] = self.getCardinality( len(selexpr)-3)
                    selexprlist.append( selexpr)
                    selexpr = ["contains", 0, 0]
                position = term.position()
            selexpr.append( [term.type(), term.value()] )
            query.defineFeature( "docfeat", [term.type(), term.value()], 1.0)

        selexpr[2] = self.getCardinality( len(selexpr)-3)
        selexprlist.append( selexpr)

        for term in words:
            prev_first,this_first = this_first,term
            query.defineFeature( "docfeat", [term.type(), term.value()], 3.0)

        query.defineFeature( "selfeat", ["union"] + selexprlist, 1.0 )

        query.setMaxNofRanks( nofranks)
        if (len(restrictdnlist) > 0):
            query.addDocumentEvaluationSet( restrictdnlist )

        # Evaluate the query:
        result = query.evaluate()
        dymitems = []

        for rank in result.ranks():
            for sumelem in rank.summaryElements():
                if sumelem.name() == 'docid':
                    sumweight = 0.0
                    weight = rank.weight()
                    occupied = []
                    elems = sumelem.value().split()
                    occupation = DymBackend.getBestElemOccuppation( terms, elems)
                    if occupation is None:
                        continue
                    dymitems.append( DymItem( sumelem.value(), occupation.weight * rank.weight()))

        dymitems.sort( key=lambda b: b.weight, reverse=True)
        weight = None
        for dymidx,dymitem in enumerate(dymitems):
           if weight is None:
               weight = dymitem.weight
           elif (weight > dymitem.weight + dymitem.weight / 2):
               dymitems = dymitems[ :dymidx]
               break

        # Get the results:
        rt = []
        for dymitem in dymitems:
            rt.append( dymitem.phrase)
        return rt


# Shutdown procedure for cleanup:
def processShutdown():
    pass;

# Server callback function that intepretes the client message sent, executes the command and packs the result for the client
@tornado.gen.coroutine
def processCommand( message):
    rt = bytearray(b"Y")
    try:
        messagesize = len(message)
        messageofs = 1
        if (message[0] == 'Q'):
            # QUERY:
            nofranks = 20
            restrictdnlist = []
            scheme = "td"
            querystr = "";
            # Build query to evaluate from the request:
            messagesize = len(message)
            while (messageofs < messagesize):
                if (message[ messageofs] == 'N'):
                    (nofranks,) = struct.unpack_from( ">H", message, messageofs+1)
                    messageofs += struct.calcsize( ">H") + 1
                elif (message[ messageofs] == 'D'):
                    (restrictdn,) = struct.unpack_from( ">I", message, messageofs+1)
                    restrictdnlist.append( restrictdn)
                    messageofs += struct.calcsize( ">I") + 1
                elif (message[ messageofs] == 'S'):
                    (strsize,) = struct.unpack_from( ">H", message, messageofs+1)
                    messageofs += struct.calcsize( ">H") + 1
                    (querystr,) = struct.unpack_from( "%ds" % (strsize), message, messageofs)
                    messageofs += strsize
                else:
                    raise tornado.gen.Return( b"Eunknown parameter")

            # Evaluate query:
            proposals = dymBackend.evaluateQuery( querystr, nofranks, restrictdnlist)

            for result in proposals:
                rt.append( '_')
                rt.append( 'P')
                rt += struct.pack( ">H%ds" % len(result), len(result), result)
        else:
            raise Exception( "unknown protocol command '%c'" % (message[0]))
    except Exception as e:
        raise tornado.gen.Return( bytearray( b"E" + str(e)) )
    raise tornado.gen.Return( rt)


# Shutdown function that sends the negative statistics to the statistics server (unsubscribe):
def processShutdown():
    pass

# Server main:
if __name__ == "__main__":
    try:
        # Parse arguments:
        defaultconfig = "path=storage_dym; cache=1G"
        parser = optparse.OptionParser()
        parser.add_option("-p", "--port", dest="port", default=7189,
                          help="Specify the port of this server as PORT (default %u)" % 7189,
                          metavar="PORT")
        parser.add_option("-c", "--config", dest="config", default=defaultconfig,
                          help="Specify the storage path as CONF (default '%s')" % defaultconfig,
                          metavar="CONF")
        parser.add_option("-G", "--debug", action="store_true", dest="do_debugtrace", default=False,
                          help="Tell the node to print some messages for tracing what it does")
        (options, args) = parser.parse_args()
        if len(args) > 0:
            parser.error("no arguments expected")
            parser.print_help()

        myport = int(options.port)
        debugtrace = options.do_debugtrace
        dymBackend = DymBackend( options.config)

        # Start server:
        print( "Starting server ...")
        server = strusMessage.RequestServer( processCommand, processShutdown)
        server.start( myport)
        print( "Terminated")
    except Exception as e:
        print( e)


