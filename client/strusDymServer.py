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
# Information retrieval engine:
dymBackend = None
# IO loop:
global debugtrace
debugtrace = False
# Strus client connection factory:
msgclient = strusMessage.RequestClient()

# Structure data types:
DymItem = collections.namedtuple('DymItem', ["name","weight"])

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
        rt.addWeightingFormula( "_0 / sqrt(_1 + 1)", {});

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

    @staticmethod
    def getCardinality( featlen):
        if (featlen >= 5):
            return 3
        if (featlen >= 2):
            return 2
        return 1

    @staticmethod
    def hasPrefixMinEditDist_( s1, p1, s2, p2, dist):
        l1 = len(s1)
        l2 = len(s2)

        while p1 < l1 and p2 < l2:
            if s1[ p1].lower() == s2[ p2].lower():
                p1 += 1
                p2 += 1
                pass
            elif dist == 0:
                return False
            # Case 1 - Replace one character in string s1 with one from s2:
            elif DymBackend.hasPrefixMinEditDist_( s1, p1+1, s2, p2+1, dist-1):
                return True
            # Case 2 - Remove one character from string s1:
            elif DymBackend.hasPrefixMinEditDist_( s1, p1+1, s2, p2, dist-1):
                return True
            # Case 3 - Remove one character from string s2:
            elif DymBackend.hasPrefixMinEditDist_( s1, p1, s2, p2+1, dist-1):
                return True
            else:
                return False
        return (p1==p2 or p2==l2) and p1==l1

    @staticmethod
    def hasPrefixMinEditDist( s1, s2, dist):
        return DymBackend.hasPrefixMinEditDist_( s1, 0, s2, 0, dist)

    @staticmethod
    def getDymCandidates( term, candidates):
        rt = []
        for cd in candidates:
            card = 2
            if len( term) < 5:
                card = 1
            if len( term) < 3:
                card = 0
            if DymBackend.hasPrefixMinEditDist( term, cd, card):
                rt.append( DymItem( cd, candidates[ cd]))
        return rt

    # Query for retrieval of 'did you mean' proposals:
    def evaluateQuery( self, querystr, nofranks):
        terms = querystr.split()
        ngrams = self.analyzer.analyzePhrase( "dym", querystr)
        if len( terms) == 0 or len(ngrams) == 0:
            # Return empty result for empty query:
            return []
        queryeval = self.queryeval_dym
        query = queryeval.createQuery( self.storage_dym)

        selexpr = ["contains", 0, 0]
        position = 0
        for term in ngrams:
            if (term.position() != position):
                if (position != 0):
                    selexpr[2] = self.getCardinality( len(selexpr)-3)
                    query.defineFeature( "selfeat", selexpr, 1.0 )
                    selexpr = ["contains", 0, 0]
                position = term.position()
            selexpr.append( [term.type(), term.value()] )
            query.defineFeature( "docfeat", [term.type(), term.value()], 1.0)

        selexpr[2] = self.getCardinality( len(selexpr)-3)
        query.defineFeature( "selfeat", selexpr, 1.0 )
        query.setMaxNofRanks( nofranks)

        # Evaluate the query:
        candidates = {}
        result = query.evaluate()
        proposals = []
        for rank in result.ranks():
            for sumelem in rank.summaryElements():
                if sumelem.name() == 'docid':
                    for elem in string.split( sumelem.value()):
                        weight = candidates.get( elem)
                        if (weight == None or weight < rank.weight()):
                            candidates[ elem] = rank.weight()

        # Get the candidates:
        for term in terms:
            proposals_tmp = []
            cdlist = self.getDymCandidates( term, candidates)
            for cd in cdlist:
                if proposals:
                    for prp in proposals:
                        proposals_tmp.append( DymItem( prp.name + " " + cd.name, cd.weight + prp.weight))
                else:
                    proposals_tmp.append( DymItem( cd.name, cd.weight))
            if not proposals_tmp:
                if proposals:
                    for prp in proposals:
                        proposals_tmp.append( DymItem( prp.name + " " + term, 0.0 + prp.weight))
                else:
                    proposals_tmp.append( DymItem( term, 0.0))
            proposals,proposals_tmp = proposals_tmp,proposals

        # Sort the result:
        proposals.sort( key=lambda b: b.weight, reverse=True)
        rt = []
        nofresults = len(proposals)
        if nofresults > 20:
            nofresults = 20
        for proposal in proposals[ :nofresults]:
            rt.append( proposal.name)
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
            scheme = "td"
            querystr = "";
            # Build query to evaluate from the request:
            messagesize = len(message)
            while (messageofs < messagesize):
                if (message[ messageofs] == 'N'):
                    (nofranks,) = struct.unpack_from( ">H", message, messageofs+1)
                    messageofs += struct.calcsize( ">H") + 1
                elif (message[ messageofs] == 'S'):
                    (strsize,) = struct.unpack_from( ">H", message, messageofs+1)
                    messageofs += struct.calcsize( ">H") + 1
                    (querystr,) = struct.unpack_from( "%ds" % (strsize), message, messageofs)
                    messageofs += strsize
                else:
                    raise tornado.gen.Return( b"Eunknown parameter")

            # Evaluate query:
            proposals = dymBackend.evaluateQuery( querystr, nofranks)

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


