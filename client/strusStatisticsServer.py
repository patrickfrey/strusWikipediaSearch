#!/usr/bin/python
import tornado.ioloop
import tornado.web
import optparse
import os
import sys
import binascii
import struct
import strus
import collections
import strusMessage

# [1] Globals:
# Term df map:
termDfMap = {}
# Collection size (number of documents):
collectionSize = 0
# Strus statistics message processor:
strusctx = strus.Context()
strusctx.defineStatisticsProcessor( "standard")
strustat = strusctx.createStatisticsProcessor()

# [2] Request handlers
def packedMessage( msg):
    return struct.pack( ">H%ds" % len(msg), len(msg), msg)

def termDfMapKey( type, value):
    return "%s~%s" % (type,value)

@tornado.gen.coroutine
def processCommand( message):
    rt = bytearray("Y")
    try:
        global collectionSize
        global termDfMap

        if (message[0] == 'P'):
            # PUBLISH:
            messagesize = len(message)
            messageofs = 1
            serverno = struct.unpack_from( ">H", message, messageofs)
            messageofs += struct.calcsize( ">H")
            msg = strustat.decode( bytearray( message[ messageofs:]))
            collectionSize += msg.nofDocumentsInsertedChange()
            dfchglist = msg.documentFrequencyChangeList()
            for dfchg in dfchglist:
                key = termDfMapKey( dfchg.type(), dfchg.value())
                if key in termDfMap:
                    termDfMap[ key ] += dfchg.increment()
                else:
                    termDfMap[ key ] = long( dfchg.increment())
        elif (message[0] == 'Q'):
            # QUERY:
            messagesize = len(message)
            messageofs = 1
            while (messageofs < messagesize):
                if (message[ messageofs] == 'T'):
                    # Fetch df of term, message format [T][typesize:16][valuesize:16][type string][value string]:
                    (typesize,valuesize) = struct.unpack_from( ">HH", message, messageofs+1)
                    messageofs += struct.calcsize( ">HH") + 1
                    (type,value) = struct.unpack_from( "%ds%ds" % (typesize,valuesize), message, messageofs)
                    messageofs += typesize + valuesize
                    df = 0
                    key = termDfMapKey( type, value)
                    if key in termDfMap:
                        df = termDfMap[ key]
                    rt += struct.pack( ">q", df)
                elif (message[ messageofs] == 'N'):
                    # Fetch N (nof documents), message format [N]:
                    messageofs += 1
                    rt += struct.pack( ">q", collectionSize)
                else:
                    raise Exception( "unknown statistics server sub command")
        else:
            raise Exception( "unknown statistics server command")
    except Exception as e:
        raise tornado.gen.Return( bytearray( b"E" + str(e)))
    raise tornado.gen.Return( rt)

def processShutdown():
    pass

# [5] Server main:
if __name__ == "__main__":
    try:
        # Parse arguments:
        parser = optparse.OptionParser()
        parser.add_option("-p", "--port", dest="port", default=7183,
                          help="Specify the port of this server as PORT (default %u)" % 7183,
                          metavar="PORT")

        (options, args) = parser.parse_args()
        if len(args) > 0:
            parser.error("no arguments expected")
            parser.print_help()
        myport = int(options.port)

        # Start server:
        print( "Starting server ...")
        server = strusMessage.RequestServer( processCommand, processShutdown)
        server.start( myport)
        print( "Terminated\n")
    except Exception as e:
        print( e)


