# Answer a DYM (did you mean) query:
class DymHandler( tornado.web.RequestHandler ):
    @tornado.gen.coroutine
    def evaluateDymQuery( self, oldquerystr, querystr, nofranks, nofresults, restrictdnlist):
        rt = ([],None)
        conn = None
        try:
            query = bytearray("Q")
            query.append('S')
            querystrsize = len( querystr)
            query += struct.pack( ">H%ds" % (querystrsize), querystrsize, querystr)
            query.append('N')
            query += struct.pack( ">H", nofranks)
            for restrictdn in restrictdnlist:
                query.append('D')
                query += struct.pack( ">I", restrictdn)

            ri = dymserver.rindex(':')
            host,port = dymserver[:ri],int( dymserver[ri+1:])
            conn = yield msgclient.connect( host, port)
            reply = yield msgclient.issueRequest( conn, query)
            if reply[0] == 'E':
                raise Exception( "failed to query dym server: %s" % reply[1:])
            elif reply[0] != 'Y':
                raise Exception( "protocol error in dym server query")
            proposals = []
            replyofs = 1
            replylen = len(reply)
            while (replyofs < replylen):
                if reply[ replyofs] == '_':
                    replyofs += 1
                elif reply[ replyofs] == 'P':
                    replyofs += 1
                    (propsize,) = struct.unpack_from( ">H", reply, replyofs)
                    replyofs += struct.calcsize( ">H")
                    (propstr,) = struct.unpack_from( "%ds" % (propsize), reply, replyofs)
                    replyofs += propsize
                    proposals.append( propstr)
                else:
                    break
            if replyofs != replylen:
                raise Exception("dym server result format error")
            rt = (proposals, None)
            conn.close()
        except Exception as e:
            errmsg = "dym server request failed: %s" % e;
            if conn:
                conn.close()
            rt = ([], errmsg)
        raise tornado.gen.Return( rt)

    @tornado.gen.coroutine
    def get(self):
        try:
            # q = query string:
            querystr = self.get_argument( "q", "").encode('utf-8')
            oldquerystr = self.get_argument( "o", "").encode('utf-8')
            # n = nofranks:
            nofranks = int( self.get_argument( "n", 20))
            # d = restrict to document:
            restrictdnlist = []
            restrictdn = int( self.get_argument( "d", 0))
            if restrictdn:
                restrictdnlist.append( restrictdn)
            # Evaluate query:
            start_time = time.time()
            (result,errormsg) = yield self.evaluateDymQuery( oldquerystr, querystr, nofranks, nofranks, restrictdnlist)
            time_elapsed = time.time() - start_time
            response = { 'error': errormsg,
                         'result': result,
                         'time': time_elapsed
            }
            self.write(response)
        except Exception as e:
            response = { 'error': str(e) }
            self.write(response)

# [2] Dispatcher:
application = tornado.web.Application([
    # /dym in the URL triggers the handler for answering queries:
    (r"/querydym", DymHandler),
])


