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
import strusMessage
import binascii
import time
import strusIR

# Information retrieval engine:
backend = None
# Port of the global statistics server:
statserver = "localhost:7183"
# Globals:
global pubstats
pubstats = False
global serverno
serverno = 1
global debugtrace
debugtrace = False
# Strus client connection factory:
msgclient = strusMessage.RequestClient()

# Call of the statustics server to publish the statistics of this storage on insert:
@tornado.gen.coroutine
def publishStatistics( itr):
    # Open connection to statistics server:
    try:
        ri = statserver.rindex(':')
        host,port = statserver[:ri],int( statserver[ri+1:])
        conn = yield msgclient.connect( host, port)
    except IOError as e:
        raise Exception( "connection to statistics server %s failed (%s)" % (statserver, e))

    msg = itr.getNext()
    while (len(msg) > 0):
        try:
            reply = yield msgclient.issueRequest( conn, b"P" + struct.pack(">H",serverno) + bytearray(msg) )
            if (reply[0] == 'E'):
                raise Exception( "error in statistics server: %s" % reply[ 1:])
            elif (reply[0] != 'Y'):
                raise Exception( "protocol error publishing statistics")
        except tornado.iostream.StreamClosedError:
            raise Exception( "unexpected close of statistics server")
        msg = itr.getNext()


# Determine if the query is only containing high frequency terms. In this case we change the retrieval scheme:
def isStopWordsOnlyQuery( terms, collectionsize):
    for term in terms:
        if term.df != 0.0 and term.df < collectionsize/12:
            return False
    return True

# Server callback function that intepretes the client message sent, executes the command and packs the result for the client
@tornado.gen.coroutine
def processCommand( message):
    rt = bytearray(b"Y")
    try:
        messagesize = len(message)
        messageofs = 1
        if message[0] == 'Q':
            # QUERY:
            Term = collections.namedtuple('Term', ['type', 'value', 'length', 'df', 'weight', 'cover'])
            nofranks = 20
            restrictdn = 0
            collectionsize = 0
            firstrank = 0
            scheme = "BM25"
            terms = []
            links = []
            with_debuginfo = False
            # Build query to evaluate from the request:
            messagesize = len(message)
            while (messageofs < messagesize):
                if message[ messageofs] == 'I':
                    (firstrank,) = struct.unpack_from( ">H", message, messageofs+1)
                    messageofs += struct.calcsize( ">H") + 1
                elif message[ messageofs] == 'N':
                    (nofranks,) = struct.unpack_from( ">H", message, messageofs+1)
                    messageofs += struct.calcsize( ">H") + 1
                elif message[ messageofs] == 'D':
                    (restrictdn,) = struct.unpack_from( ">I", message, messageofs+1)
                    messageofs += struct.calcsize( ">I") + 1
                elif message[ messageofs] == 'M':
                    (scheme,messageofs) = strusMessage.unpackString( message, messageofs+1)
                elif message[ messageofs] == 'S':
                    (collectionsize,) = struct.unpack_from( ">q", message, messageofs+1)
                    messageofs += struct.calcsize( ">q") + 1
                elif message[ messageofs] == 'T':
                    (type, messageofs) = strusMessage.unpackString( message, messageofs+1)
                    (value, messageofs) = strusMessage.unpackString( message, messageofs)
                    (length,df,weight,cover) = struct.unpack_from( ">Hqd?", message, messageofs)
                    messageofs += struct.calcsize( ">Hqd?")
                    terms.append( Term( type, value, length, df, weight, cover))
                elif message[ messageofs] == 'L':
                    (type, messageofs) = strusMessage.unpackString( message, messageofs+1)
                    (value, messageofs) = strusMessage.unpackString( message, messageofs)
                    (weight,) = struct.unpack_from( ">d", message, messageofs)
                    messageofs += struct.calcsize( ">d")
                    links.append( Term( type, value, 1, 0, weight, False))
                elif message[ messageofs] == 'B':
                    messageofs += 1
                    with_debuginfo = True
                else:
                    raise tornado.gen.Return( b"Eunknown parameter")

            doTitleSelect = isStopWordsOnlyQuery( terms, collectionsize)
            # ... if we have a query containing only stopwords, we reduce our search space to 
            # the documents containing some query terms in the title and the most referenced
            # documents in the collection.

            # Evaluate query:
            if restrictdn == 0:
                results = backend.evaluateQuery( scheme, doTitleSelect, terms, links, collectionsize, firstrank, nofranks, [], debugtrace, with_debuginfo)
            else:
                results = backend.evaluateQuery( scheme, doTitleSelect, terms, links, collectionsize, firstrank, nofranks, [restrictdn], debugtrace, with_debuginfo)

            # Build the result and pack it into the reply message for the client:
            rt.append( 'Z')
            rt += struct.pack( ">H", serverno)

            if scheme == "NBLNK" or scheme == "TILNK" or scheme == "VCLNK":
                for result in results:
                    rt.append( '_')
                    rt.append( 'D')
                    rt += struct.pack( ">I", result.docno)
                    rt.append( 'W')
                    rt += struct.pack( ">d", result.weight)
                    for linkid,weight in result.links:
                        rt.append( 'L')
                        rt += strusMessage.packString( linkid) + struct.pack( ">d", weight)
            elif scheme == "STDLNK":
                for result in results:
                    rt.append( '_')
                    rt.append( 'D')
                    rt += struct.pack( ">I", result.docno)
                    rt.append( 'W')
                    rt += struct.pack( ">d", result.weight)
                    for linkid,weight in result.links:
                        rt.append( 'L')
                        rt += strusMessage.packString( linkid) + struct.pack( ">d", weight)
                    for linkid,weight in result.titles:
                        rt.append( 'T')
                        rt += strusMessage.packString( linkid) + struct.pack( ">d", weight)
                    for featid,weight in result.features:
                        rt.append( 'F')
                        rt += strusMessage.packString( featid) + struct.pack( ">d", weight)
            else:
                for result in results:
                    rt.append( '_')
                    rt.append( 'D')
                    rt += struct.pack( ">I", result.docno)
                    rt.append( 'W')
                    rt += struct.pack( ">d", result.weight)
                    rt.append( 'T')
                    rt += strusMessage.packString( result.title)
                    if result.paratitle:
                        rt.append( 'P')
                        rt += strusMessage.packString( result.paratitle)
                    if result.debuginfo:
                        rt.append( 'B')
                        rt += strusMessage.packString( result.debuginfo)
                    rt.append( 'A')
                    rt += strusMessage.packString( result.abstract)
        else:
            raise Exception( "unknown protocol command '%c'" % (message[0]))
    except Exception as e:
        raise tornado.gen.Return( bytearray( b"E" + str(e)) )
    raise tornado.gen.Return( rt)


# Shutdown function that sends the negative statistics to the statistics server (unsubscribe):
def processShutdown():
    global pubstats
    if (pubstats):
        pubstats = False
# !--- The following code does not work because the server shuts down before publishing the df decrements:
#        publishStatistics( backend.getDoneStatisticsIterator())

# Server main:
if __name__ == "__main__":
    try:
        # Parse arguments:
        defaultconfig = "path=storage; cache=512M; statsproc=default"
        parser = optparse.OptionParser()
        parser.add_option("-p", "--port", dest="port", default=7184,
                          help="Specify the port of this server as PORT (default %u)" % 7184,
                          metavar="PORT")
        parser.add_option("-c", "--config", dest="config", default=defaultconfig,
                          help="Specify the storage config as CONF (default '%s')" % defaultconfig,
                          metavar="CONF")
        parser.add_option("-i", "--serverno", dest="serverno", default=serverno,
                          help="Specify the number of the storage node as NO (default %s)" % serverno,
                          metavar="NO")
        parser.add_option("-s", "--statserver", dest="statserver", default=statserver,
                          help="Specify the address of the statistics server as ADDR (default %s)" % statserver,
                          metavar="ADDR")
        parser.add_option("-P", "--publish-stats", action="store_true", dest="do_publish_stats", default=False,
                          help="Tell the node to publish the own storage statistics to the statistics server at startup")
        parser.add_option("-G", "--debug", action="store_true", dest="do_debugtrace", default=False,
                          help="Tell the node to print some messages for tracing what it does")
        (options, args) = parser.parse_args()
        if len(args) > 0:
            parser.error("no arguments expected")
            parser.print_help()

        myport = int(options.port)
        pubstats = options.do_publish_stats
        debugtrace = options.do_debugtrace
        statserver = options.statserver
        serverno = int( options.serverno)
        backend = strusIR.Backend( options.config)

        if (statserver[0:].isdigit()):
            statserver = '{}:{}'.format( 'localhost', statserver)

        if (pubstats):
            # Start publish local statistics:
            print "Load local statistics to publish (serverno %u) ..." % serverno
            publishStatistics( backend.getInitStatisticsIterator())

        # Start server:
        print "Starting server ..."
        server = strusMessage.RequestServer( processCommand, processShutdown)
        server.start( myport)
        print "Terminated"
    except Exception as e:
        print e


