/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "textwolf/istreamiterator.hpp"
#include "textwolf/xmlscanner.hpp"
#include "textwolf/charset.hpp"
#include "strus/lib/error.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/base/fileio.hpp"
#include "strus/base/thread.hpp"
#include "strus/base/numstring.hpp"
#include "strus/base/inputStream.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/atomic.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/base/string_conv.hpp"
#include "strus/errorBufferInterface.hpp"
#include "linkMap.hpp"
#include "documentStructure.hpp"
#include "outputString.hpp"
#include "wikimediaLexer.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <string>
#include <stdexcept>
#include <memory>
#include <vector>
#include <set>
#include <queue>
#include <limits>

static int g_verbosity = 0;
static bool g_beautified = false;
static bool g_dumps = false;
static bool g_singleIdAttribute = true;
static bool g_dumpStdout = false;
static bool g_doTest = false;
static std::string g_testExpectedFilename;
static std::string g_testOutput;
static std::string g_outputdir;
static const strus::LinkMap* g_linkmap = NULL;
static strus::ErrorBufferInterface* g_errorhnd = NULL;

typedef textwolf::XMLScanner<textwolf::IStreamIterator,textwolf::charset::UTF8,textwolf::charset::UTF8,std::string> XmlScanner;

static std::string attributesToString( const strus::WikimediaLexem::AttributeMap& attributes)
{
	std::ostringstream out;
	strus::WikimediaLexem::AttributeMap::const_iterator ai = attributes.begin(), ae = attributes.end();
	for (int aidx=0; ai != ae; ++ai,++aidx)
	{
		if (aidx) out << ", ";
		out << ai->first << "='" << ai->second << "'";
	}
	return out.str();
}

static std::string getLinkDomainPrefix( const std::string& lnk, int maxsize)
{
	char const* si = lnk.c_str();
	while (maxsize > 0 && (*si|32) >= 'a' && (*si|32) <= 'z') {++si;--maxsize;}
	if (*si == ':')
	{
		return strus::string_conv::tolower( lnk.c_str(), si - lnk.c_str());
	}
	else
	{
		return std::string();
	}
}

static void parseDocumentText( strus::DocumentStructure& doc, const char* src, std::size_t size)
{
	strus::WikimediaLexer lexer(src,size);
	int lexemidx = 0;
	int lastHeading = 1;
	bool verboseOutput = (g_verbosity >= 2);

	for (strus::WikimediaLexem lexem = lexer.next(); lexem.id != strus::WikimediaLexem::EoF; lexem = lexer.next(),++lexemidx)
	{
		if (verboseOutput)
		{
			std::cout << "STATE " << doc.statestring() << std::endl;
			std::cout << lexemidx << " LEXEM " << strus::WikimediaLexem::idName( lexem.id) << " " << strus::outputLineString( lexem.value.c_str(), lexem.value.c_str() + lexem.value.size());
			if (!lexem.attributes.empty()) std::cout << " -- " << attributesToString( lexem.attributes);
			std::cout << std::endl;
		}
		switch (lexem.id)
		{			
			case strus::WikimediaLexem::EoF:
				break;
			case strus::WikimediaLexem::Error:
				doc.addError( std::string("syntax error in document: ") + strus::outputLineString( lexem.value.c_str()));
				break;
			case strus::WikimediaLexem::Text:
				doc.addText( lexem.value);
				break;
			case strus::WikimediaLexem::String:
				doc.closeOpenQuoteItems();
				doc.addQuotationMarker();
				doc.addText( lexem.value);
				doc.addQuotationMarker();
				break;
			case strus::WikimediaLexem::Char:
				doc.addChar( lexem.value);
				break;
			case strus::WikimediaLexem::Math:
				doc.addMath( lexem.value);
				break;
			case strus::WikimediaLexem::BibRef:
				doc.addBibRef( lexem.value);
				break;
			case strus::WikimediaLexem::NoWiki:
				doc.addNoWiki( lexem.value);
				break;
			case strus::WikimediaLexem::NoData:
				doc.addError( std::string("lexem can not be treated as data: ") + strus::outputLineString( lexem.value.c_str()));
				break;
			case strus::WikimediaLexem::Code:
				doc.addCode( lexem.value);
				break;
			case strus::WikimediaLexem::Timestamp:
				doc.addTimestamp( lexem.value);
				break;
			case strus::WikimediaLexem::Url:
				doc.openWebLink( lexem.value);
				doc.closeWebLink();
				break;
			case strus::WikimediaLexem::Redirect:
				doc.addError( "unexpected redirect in document");
				break;
			case strus::WikimediaLexem::Markup:
				doc.addMarkup( lexem.value);
				break;
			case strus::WikimediaLexem::OpenHeading:
				doc.openHeading( lastHeading = (int)lexem.idx);
				break;
			case strus::WikimediaLexem::CloseHeading:
				doc.closeHeading();
				break;
			case strus::WikimediaLexem::OpenRef:
				doc.openRef();
				break;
			case strus::WikimediaLexem::CloseRef:
				doc.closeRef();
				break;
			case strus::WikimediaLexem::HeadingItem:
				doc.addHeadingItem();
				break;
			case strus::WikimediaLexem::ListItem:
				doc.openListItem( (int)lexem.idx);
				break;
			case strus::WikimediaLexem::EndOfLine:
				doc.closeOpenEolnItem();
				doc.addBreak();
				break;
			case strus::WikimediaLexem::QuotationMarker:
				doc.addQuotationMarker();
				break;
			case strus::WikimediaLexem::MultiQuoteMarker:
				doc.addMultiQuoteMarker( (int)lexem.idx);
				break;
			case strus::WikimediaLexem::OpenSpan:
				doc.openSpan();
				break;
			case strus::WikimediaLexem::CloseSpan:
				doc.closeSpan();
				break;
			case strus::WikimediaLexem::OpenFormat:
				doc.openFormat();
				break;
			case strus::WikimediaLexem::CloseFormat:
				doc.closeFormat();
				break;
			case strus::WikimediaLexem::OpenBlockQuote:
				doc.openBlockQuote();
				break;
			case strus::WikimediaLexem::CloseBlockQuote:
				doc.closeBlockQuote();
				break;
			case strus::WikimediaLexem::OpenDiv:
				doc.openDiv();
				break;
			case strus::WikimediaLexem::CloseDiv:
				doc.closeDiv();
				break;
			case strus::WikimediaLexem::OpenPoem:
				doc.openPoem();
				break;
			case strus::WikimediaLexem::ClosePoem:
				doc.closePoem();
				break;
			case strus::WikimediaLexem::OpenCitation:
				doc.openCitation( lexem.value);
				break;
			case strus::WikimediaLexem::CloseCitation:
				doc.closeCitation();
				break;
			case strus::WikimediaLexem::OpenWWWLink:
				doc.openWebLink( lexem.value);
				break;
			case strus::WikimediaLexem::CloseWWWLink:
				doc.closeWebLink();
				break;
			case strus::WikimediaLexem::OpenPageLink:
			{
				std::pair<std::string,std::string> lnk = strus::LinkMap::getLinkParts( lexem.value);
				std::string link = lnk.first;
				std::string anchorid;
				if (lnk.second.size() > 80)
				{
					doc.setLinkDescription( lnk.second);
				}
				else
				{
					anchorid = lnk.second;
				}

				std::string prefix = getLinkDomainPrefix( link, 12);
				if (strus::caseInsensitiveEquals( prefix, "wikipedia"))
				{
					link = strus::string_conv::trim( link.c_str() + prefix.size()+1);
				}
				if (strus::caseInsensitiveEquals( prefix, "file")
				||  strus::caseInsensitiveEquals( prefix, "image")
				||  strus::caseInsensitiveEquals( prefix, "category"))
				{
					doc.openPageLink( link, anchorid);
				}
				else
				{
					if (g_linkmap)
					{
						const char* val = g_linkmap->get( link);
						if (!val)
						{
							doc.addUnresolved( lexem.value);
							doc.openPageLink( val, anchorid);
						}
						else
						{
							doc.openPageLink( link, anchorid);
						}
					} else
					{
						doc.openPageLink( link, anchorid);
					}
				}
				break;
			}
			case strus::WikimediaLexem::ClosePageLink:
				doc.closePageLink();
				break;
			case strus::WikimediaLexem::OpenTable:
				doc.openTable();
				break;
			case strus::WikimediaLexem::CloseTable:
				doc.closeOpenEolnItem();
				doc.closeTable();
				break;
			case strus::WikimediaLexem::TableTitle:
				doc.closeOpenEolnItem();
				doc.implicitOpenTableIfUndefined();
				doc.addTableTitle();
				break;
			case strus::WikimediaLexem::TableHeadDelim:
			{
				doc.closeOpenEolnItem();
				doc.implicitOpenTableIfUndefined();
				int colspan = lexem.colspan();
				if (colspan <= 0)
				{
					doc.addError( "invalid colspan attribute value");
					colspan = 0;
				}
				int rowspan = lexem.rowspan();
				if (rowspan <= 0)
				{
					doc.addError( "invalid colspan attribute value");
					rowspan = 0;
				}
				doc.addTableHead( rowspan, colspan);
				break;
			}
			case strus::WikimediaLexem::TableRowDelim:
				doc.closeOpenEolnItem();
				doc.implicitOpenTableIfUndefined();
				doc.addTableRow();
				break;
			case strus::WikimediaLexem::TableColDelim:
			{
				doc.closeOpenEolnItem();
				strus::Paragraph::StructType tp = doc.currentStructType();
				if (tp == strus::Paragraph::StructPageLink
				||  tp == strus::Paragraph::StructWebLink)
				{
					doc.clearOpenText();
					//... ignore last text and restart structure
				}
				else
				if (tp == strus::Paragraph::StructCitation
				||  tp == strus::Paragraph::StructRef
				||  tp == strus::Paragraph::StructAttribute)
				{
					doc.addAttribute( lexem.value);
				}
				else if (tp == strus::Paragraph::StructNone)
				{
					doc.openListItem( 1);
				}
				else
				{
					int colspan = lexem.colspan();
					if (colspan <= 0)
					{
						doc.addError( "invalid colspan attribute value");
						colspan = 0;
					}
					int rowspan = lexem.rowspan();
					if (rowspan <= 0)
					{
						doc.addError( "invalid colspan attribute value");
						rowspan = 0;
					}
					doc.addTableCell( rowspan, colspan);
				}
				break;
			}
			case strus::WikimediaLexem::ColDelim:
			{
				doc.closeOpenQuoteItems();
				strus::Paragraph::StructType tp = doc.currentStructType();
				if (tp == strus::Paragraph::StructPageLink
				||  tp == strus::Paragraph::StructWebLink)
				{
					doc.clearOpenText();
					//... ignore last text and restart structure
				}
				else if (tp == strus::Paragraph::StructList)
				{
					doc.addText( " |");
					//... ignore
				}
				else if (tp == strus::Paragraph::StructTableTitle)
				{
					doc.addTableTitle();
				}
				else if (tp == strus::Paragraph::StructTableHead
					|| tp == strus::Paragraph::StructTableCell)
				{
					int colspan = lexem.colspan();
					if (colspan <= 0)
					{
						doc.addError( "invalid colspan attribute value");
						colspan = 0;
					}
					int rowspan = lexem.rowspan();
					if (rowspan <= 0)
					{
						doc.addError( "invalid colspan attribute value");
						rowspan = 0;
					}
					doc.repeatTableCell( rowspan, colspan);
				}
				else
				{
					doc.addAttribute( lexem.value);
				}
				break;
			}
			case strus::WikimediaLexem::DoubleColDelimNewLine:
			case strus::WikimediaLexem::DoubleColDelim:
			{
				doc.disableOpenFormatAndQuotes();
				strus::Paragraph::StructType tp = doc.currentStructType();
				if (tp == strus::Paragraph::StructList && lexem.id == strus::WikimediaLexem::DoubleColDelimNewLine)
				{
					doc.closeAutoCloseItem( strus::Paragraph::ListItemStart);
					tp = doc.currentStructType();
				}
				if (tp == strus::Paragraph::StructPageLink
				||  tp == strus::Paragraph::StructWebLink)
				{
					doc.clearOpenText();
					//... ignore last text and restart structure
				}
				else if (tp == strus::Paragraph::StructTableTitle)
				{
					doc.addTableTitle();
				}
				else if (tp == strus::Paragraph::StructTableHead
					|| tp == strus::Paragraph::StructTableCell)
				{
					int colspan = lexem.colspan();
					if (colspan <= 0)
					{
						doc.addError( "invalid colspan attribute value");
						colspan = 0;
					}
					int rowspan = lexem.rowspan();
					if (rowspan <= 0)
					{
						doc.addError( "invalid colspan attribute value");
						rowspan = 0;
					}
					doc.repeatTableCell( rowspan, colspan);
				}
				else if (tp == strus::Paragraph::StructCitation || tp == strus::Paragraph::StructAttribute)
				{
					doc.addAttribute( lexem.value);
				}
				else if (tp == strus::Paragraph::StructTable)
				{
					int colspan = lexem.colspan();
					if (colspan <= 0)
					{
						doc.addError( "invalid colspan attribute value");
						colspan = 0;
					}
					int rowspan = lexem.rowspan();
					if (rowspan <= 0)
					{
						doc.addError( "invalid colspan attribute value");
						rowspan = 0;
					}
					doc.addTableCell( rowspan, colspan);
				}
				else if (tp == strus::Paragraph::StructList)
				{
					doc.addBreak();
				}
				else
				{
					doc.addError( "unexpected token '||'");
				}
			}
			case strus::WikimediaLexem::Break:
				doc.addBreak();
		}
		if (doc.hasNewErrors())
		{
			doc.setErrorsSourceInfo( lexer.currentSourceExtract( 60));
		}
		
	}
}

static void createOutputDir( int fileCounter)
{
	char dirnam[ 16];
	std::snprintf( dirnam, sizeof(dirnam), "%04u", fileCounter / 1000);
	std::string dirpath( strus::joinFilePath( g_outputdir, dirnam));
	int ec = strus::createDir( dirpath, false/*fail if exists*/);
	if (ec) std::cerr << "error creating directory " << dirpath << ": " << std::strerror(ec) << std::endl;
}

static std::string getFilenameFromDocid( int fileCounter, const std::string& docid)
{
	if (docid.size() < 120)
	{
		return docid;
	}
	else
	{
		int sz = 110;
		char const* si = docid.c_str() + sz;
		while (*si && (*(si-1) == '%' || *(si-2) == '%')) {++si,++sz;}
		return std::string( docid.c_str(), sz) + "__" + strus::string_format( "%d", fileCounter);
	}
}

static void writeWorkFile( int fileCounter, const std::string& docid, const std::string& extension, const std::string& content)
{
	char dirnam[ 16];
	std::snprintf( dirnam, sizeof(dirnam), "%04u", fileCounter / 1000);
	int ec;

	if (g_dumpStdout || g_doTest)
	{
		std::string filename( strus::joinFilePath( dirnam, getFilenameFromDocid( fileCounter, docid) + extension));
		if (g_dumpStdout)
		{
			std::cout << "## " << filename << std::endl;
			std::cout << content << std::endl << std::endl;
		}
		else
		{
			std::ostringstream out;
			out << "## " << filename << std::endl;
			out << content << std::endl << std::endl;
			g_testOutput.append( out.str());
		}
	}
	else
	{
		std::string filename( strus::joinFilePath( strus::joinFilePath( g_outputdir, dirnam), getFilenameFromDocid( fileCounter, docid) + extension));
		ec = strus::writeFile( filename, content);
		if (ec) std::cerr << "error writing file " << filename << ": " << std::strerror(ec) << std::endl;
	}
}

static void removeWorkFile( int fileCounter, const std::string& docid, const std::string& extension)
{
	if (g_dumpStdout || g_doTest) return;

	char dirnam[ 16];
	std::snprintf( dirnam, sizeof(dirnam), "%04u", fileCounter / 1000);
	int ec;

	std::string filename( strus::joinFilePath( strus::joinFilePath( g_outputdir, dirnam), getFilenameFromDocid( fileCounter, docid) + extension));
	ec = strus::removeFile( filename, false);
	if (ec) std::cerr << "error removing file " << filename << ": " << std::strerror(ec) << std::endl;
}

static void writeErrorFile( int fileCounter, const std::string& docid, const std::string& errorstext)
{
	writeWorkFile( fileCounter, docid, ".err", errorstext);
	if (g_verbosity >= 1) std::cerr << "got error:" << std::endl << errorstext << std::endl;
}

static void writeFatalErrorFile( int fileCounter, const std::string& docid, const std::string& errorstext)
{
	writeWorkFile( fileCounter, docid, ".ftl", errorstext);
	if (g_verbosity >= 1) std::cerr << "got fatal error:" << std::endl << errorstext << std::endl;
}

static void writeInputFile( int fileCounter, const std::string& docid, const std::string& title, const std::string& content)
{
	std::string origxml = strus::DocumentStructure::getInputXML( title, content);
	writeWorkFile( fileCounter, docid, ".org", origxml);
}

static void writeLexerDumpFile( int fileCounter, const strus::DocumentStructure& doc)
{
	writeWorkFile( fileCounter, doc.fileId(), ".txt", doc.tostring());
}

static void writeOutputFiles( int fileCounter, const strus::DocumentStructure& doc)
{
	writeWorkFile( fileCounter, doc.fileId(), ".xml", doc.toxml( g_beautified, g_singleIdAttribute));
	std::string strange = doc.reportStrangeFeatures();
	if (strange.empty())
	{
		removeWorkFile( fileCounter, doc.fileId(), ".wtf");
	}
	else
	{
		writeWorkFile( fileCounter, doc.fileId(), ".wtf", strange);
	}
	if (doc.errors().empty())
	{
		removeWorkFile( fileCounter, doc.fileId(), ".err");
	}
	else
	{
		std::ostringstream errorstext;
		std::vector<std::string>::const_iterator ei = doc.errors().begin(), ee = doc.errors().end();
		for (int eidx=1; ei != ee; ++ei,++eidx)
		{
			errorstext << "[" << eidx << "] " << *ei << "\n";
		}
		std::string errdump( errorstext.str());
		writeWorkFile( fileCounter, doc.fileId(), ".err", errdump);
		if (g_verbosity >= 1) std::cerr << "got errors:" << std::endl << errdump << std::endl;
	}
	std::vector<std::string> unresolved( doc.unresolved());
	if (unresolved.empty())
	{
		removeWorkFile( fileCounter, doc.fileId(), ".mis");
	}
	else
	{
		std::ostringstream unresolvedtext;
		std::vector<std::string>::const_iterator ei = unresolved.begin(), ee = unresolved.end();
		for (int eidx=1; ei != ee; ++ei,++eidx)
		{
			unresolvedtext << "[" << eidx << "] " << *ei << "\n";
		}
		std::string unresolveddump( unresolvedtext.str());
		writeWorkFile( fileCounter, doc.fileId(), ".mis", unresolveddump);
		if (g_verbosity >= 1) std::cerr << "got " << (int)unresolved.size() << " unresolved page links:" << std::endl;
	}
}

class Work
{
public:
	Work()
		:m_writeDumpsAlways(false),m_fileindex(-1),m_title(),m_content(){}
	Work( int fileindex_, const std::string& title_, const std::string& content_, bool writeDumpsAlways_)
		:m_writeDumpsAlways(writeDumpsAlways_),m_fileindex(fileindex_),m_title(title_),m_content(content_){}
	Work( const Work& o)
		:m_writeDumpsAlways(o.m_writeDumpsAlways),m_fileindex(o.m_fileindex),m_title(o.m_title),m_content(o.m_content){}

	bool empty() const
	{
		return m_content.empty();
	}
	int fileindex() const				{return m_fileindex;}
	const std::string& title() const		{return m_title;}
	const std::string& content() const		{return m_content;}

	void process()
	{
		bool inputFileWritten = false;
		strus::DocumentStructure doc;
		doc.setTitle( m_title);
		try
		{
			parseDocumentText( doc, m_content.c_str(), m_content.size());
			doc.finish();
			writeOutputFiles( m_fileindex, doc);
			if (m_writeDumpsAlways || !doc.errors().empty())
			{
				writeLexerDumpFile( m_fileindex, doc);
				if (!inputFileWritten)
				{
					writeInputFile( m_fileindex, doc.fileId(), m_title, m_content);
					inputFileWritten = true;
				}
			}
		}
		catch (const std::runtime_error& err)
		{
			writeLexerDumpFile( m_fileindex, doc);
			writeErrorFile( m_fileindex, doc.fileId(), err.what());
			writeFatalErrorFile( m_fileindex, doc.fileId(), std::string(err.what()) + "\n");
			if (!inputFileWritten)
			{
				writeInputFile( m_fileindex, doc.fileId(), m_title, m_content);
				inputFileWritten = true;
			}
		}
	}

private:
	bool m_writeDumpsAlways;
	int m_fileindex;
	std::string m_title;
	std::string m_content;
};

class Worker
{
public:
	Worker()
		:m_thread(0),m_threadid(0),m_terminated(false),m_eof(false),m_writeDumpsAlways(g_dumps){}
	~Worker()
	{
		waitTermination();
	}

	void push( int filecounter, const std::string& title, const std::string& content)
	{
		strus::unique_lock lock( m_queue_mutex);
		m_queue.push( Work( filecounter, title, content, m_writeDumpsAlways));
		m_cv.notify_one();
	}
	void terminate()
	{
		m_terminated.set( true);
		m_cv.notify_all();
	}
	void waitTermination()
	{
		if (m_thread)
		{
			m_eof.set( true);
			m_cv.notify_all();
			m_thread->join();
			delete m_thread;
			m_thread = 0;
			if (g_verbosity >= 1) std::cerr << strus::string_format( "thread %d terminated\n", m_threadid) << std::flush;
		}
	}
	void waitSignal()
	{
		strus::unique_lock lock( m_cv_mutex);
		m_cv.wait( lock);
	}
	bool fetch( Work& work)
	{
		strus::unique_lock lock( m_queue_mutex);
		if (m_queue.empty())
		{
			if (m_eof.test())
			{
				m_terminated.set( true);
			}
			return false;
		}
		else
		{
			work = m_queue.front();
			m_queue.pop();
			return true;
		}
	}

	void run()
	{
		if (g_verbosity >= 1) std::cerr << strus::string_format( "thread %d started\n", m_threadid) << std::flush;
		while (!m_terminated.test())
		{
			std::string title;
			try
			{
				waitSignal();
				Work work;
				while (fetch( work))
				{
					title = work.title();
					if (g_verbosity >= 1) std::cerr << strus::string_format( "thread %d process document '%s'\n", m_threadid, title.c_str()) << std::flush;
					work.process();
				}
			}
			catch (const std::bad_alloc&)
			{
				std::cerr << "out of memory processing document " << title << std::endl;
			}
			catch (const std::runtime_error& err)
			{
				std::cerr << "error processing document " << title << ": " << err.what() << std::endl;
			}
		}
	}
	void start( int threadid_)
	{
		m_threadid = threadid_;
		if (m_thread) throw std::runtime_error("start called twice");
		m_thread = new strus::thread( &Worker::run, this);
	}

private:
	strus::condition_variable m_cv;
	strus::mutex m_cv_mutex;
	strus::mutex m_queue_mutex;
	std::queue<Work> m_queue;
	strus::thread* m_thread;
	int m_threadid;
	strus::AtomicFlag m_terminated;
	strus::AtomicFlag m_eof;
	bool m_writeDumpsAlways;
};

class IStream
	:public textwolf::IStream
{
public:
	explicit IStream( const std::string& docpath)
		:m_impl(docpath)
	{
		int ec = m_impl.error();
		if (ec) throw std::runtime_error( strus::string_format("failed to read input file '%s': %s", docpath.c_str(), ::strerror(ec)));
	}
	virtual ~IStream(){}

	virtual std::size_t read( void* buf, std::size_t bufsize)
	{
		return m_impl.read( (char*)buf, bufsize);
	}

	virtual int errorcode() const
	{
		return m_impl.error();
	}

private:
	strus::InputStream m_impl;
};

static int getUIntOptionArg( int argi, int argc, const char* argv[])
{
	if (argv[argi+1])
	{
		return strus::numstring_conv::touint( argv[argi+1], std::numeric_limits<int>::max());
	}
	else
	{
		throw std::runtime_error( std::string("no argument given for option ") + argv[argi]);
	}
}

enum TagId {TagIgnored,TagPage,TagNs,TagTitle,TagText,TagRedirect};


int main( int argc, const char* argv[])
{
	int rt = 0;
	try
	{
		int argi = 1;
		int nofThreads = 0;
		int counterMod = 0;
		std::set<int> namespacemap;
		bool namespaceset = false;
		bool printusage = false;
		bool collectRedirects = false;
		bool loadRedirects = false;
		std::string linkmapfilename;
		std::string dumpfilename;
		std::vector<std::string> selectDocumentPattern;

		for (;argi < argc; ++argi)
		{
			if (0==std::strcmp(argv[argi],"-VV"))
			{
				g_verbosity += 2;
			}
			else if (0==std::strcmp(argv[argi],"-V"))
			{
				++g_verbosity;
			}
			else if (0==std::strcmp(argv[argi],"-B"))
			{
				g_beautified = true;
			}
			else if (0==std::strcmp(argv[argi],"-D"))
			{
				g_dumps = true;
			}
			else if (0==std::strcmp(argv[argi],"-K"))
			{
				if (!dumpfilename.empty()) throw std::runtime_error("duplicated option -K <filename>");
				++argi;
				if (argi == argc || (argv[argi][0] == '-' && argv[argi][1] != '\0')) throw std::runtime_error( "option -K without argument");
				dumpfilename = argv[ argi];
			}
			else if (0==std::strcmp(argv[argi],"-I"))
			{
				g_singleIdAttribute = false;
			}
			else if (0==std::strcmp(argv[argi],"-h"))
			{
				printusage = true;
			}
			else if (0==std::memcmp(argv[argi],"-P",2))
			{
				if (counterMod > 0) throw std::runtime_error( "duplicated option -P <mod>");
				counterMod = getUIntOptionArg( argi, argc, argv);
				if (!counterMod) throw std::runtime_error( "option -P requires positive integer as argument");
				++argi;
			}
			else if (0==std::memcmp(argv[argi],"-S",2))
			{
				++argi;
				if (argi == argc || (argv[argi][0] == '-' && argv[argi][1] != '\0')) throw std::runtime_error( "option -S without argument");
				selectDocumentPattern.push_back( argv[ argi]);
			}
			else if (0==std::memcmp(argv[argi],"-L",2))
			{
				if (!linkmapfilename.empty()) throw std::runtime_error("duplicate or conflicting option -L <linkmapfile> or -R <linkmapfile>");
				++argi;
				if (argi == argc || (argv[argi][0] == '-' && argv[argi][1] != '\0')) throw std::runtime_error( "option -L without argument");
				linkmapfilename = argv[ argi];
				loadRedirects = true;
			}
			else if (0==std::memcmp(argv[argi],"-R",2))
			{
				if (!linkmapfilename.empty()) throw std::runtime_error( "duplicate or conflicting option -L <linkmapfile> or -R <linkmapfile>");
				++argi;
				if (argi == argc || (argv[argi][0] == '-' && argv[argi][1] != '\0')) throw std::runtime_error( "option -R without argument");
				linkmapfilename = argv[ argi];
				collectRedirects = true;
			}
			else if (0==std::memcmp(argv[argi],"-n",2))
			{
				namespaceset = true;
				namespacemap.insert( getUIntOptionArg( argi, argc, argv));
				++argi;
			}
			else if (0==std::memcmp(argv[argi],"-t",2))
			{
				nofThreads = getUIntOptionArg( argi, argc, argv);
				++argi;
			}
			else if (0==std::strcmp(argv[argi],"--stdout"))
			{
				g_dumpStdout = true;
			}
			else if (0==std::strcmp(argv[argi],"--test"))
			{
				if (!g_testExpectedFilename.empty()) throw std::runtime_error( "duplicate or option --test <expected file name>");
				++argi;
				if (argi == argc || (argv[argi][0] == '-' && argv[argi][1] != '\0')) throw std::runtime_error( "option --test without argument");
				g_testExpectedFilename = argv[ argi];
				g_doTest = true;
			}
			else if (argv[argi][0] == '-' && argv[argi][1] == '-')
			{
				++argi;
				break;
			}
			else if (argv[argi][0] == '-' && argv[argi][1])
			{
				std::cerr << "unknown option '" << argv[argi] << "'" << std::endl;
				printusage = true;
			}
			else
			{
				break;
			}
		}
		if (argc > argi+2 || argc < argi+1)
		{
			if (argc > argi+2) std::cerr << "too many arguments" << std::endl;
			if (argc < argi+2) std::cerr << "too few arguments" << std::endl;
			printusage = true;
			rt = -1;
		}
		if (printusage)
		{
			std::cerr << "Usage: strusWikimediaToXml [options] <inputfile> [<outputdir>]" << std::endl;
			std::cerr << "<inputfile>   :File to process or '-' for stdin" << std::endl;
			std::cerr << "<outputdir>   :Directory where output files and directories are written to." << std::endl;
			std::cerr << "options:" << std::endl;
			std::cerr << "    -h           :Print this usage" << std::endl;
			std::cerr << "    -V           :Verbosity level 1 (output document title and errors to stderr)" << std::endl;
			std::cerr << "    -VV          :Verbosity level 2 (output lexems found additional to level 1)" << std::endl;
			std::cerr << "    -S <doc>     :Select processed documents containing <doc> as title sub string" << std::endl;
			std::cerr << "    -B           :Beautified readable XML output" << std::endl;
			std::cerr << "    -P <mod>     :Print progress counter modulo <mod> to stderr" << std::endl;
			std::cerr << "    -D           :Write dump files always, not only in case of an error" << std::endl;
			std::cerr << "    -K <filename>:Write dump file to file <filename> before processing it." << std::endl;
			std::cerr << "    -t <threads> :Number of conversion threads to use is <threads>" << std::endl;
			std::cerr << "                  Total number of threads is <threads> +1" << std::endl;
			std::cerr << "                  (conversion threads + main thread)" << std::endl;
			std::cerr << "    -n <ns>      :Reduce output to namespace <ns> (0=article)" << std::endl;
			std::cerr << "    -I           :Produce one 'id' attribute per table cell reference," << std::endl;
			std::cerr << "                  instead of one with the ids separated by commas (e.g. id='C1,R2')." << std::endl;
			std::cerr << "                  One 'id' attribute per table cell reference is non valid XML," << std::endl;
			std::cerr << "                  but you should use this format if you process the XML with strus." << std::endl;
			std::cerr << "    -R <lnkfile> :Collect redirects only and write them to <lnkfile>" << std::endl;
			std::cerr << "    -L <lnkfile> :Load link file <lnkfile> for verifying page links" << std::endl;
			std::cerr << "    --stdout     :Write all output to stdout" << std::endl;
			std::cerr << "    --test <EXP> :Write all output to a string and compare it with the content" << std::endl;
			std::cerr << "                  of the file <EXP> (single threaded only)" << std::endl;
			std::cerr << std::endl;
			std::cerr << "Description:" << std::endl;
			std::cerr << "  Takes a unpacked Wikipedia XML dump as input and tries to convert it to\n";
			std::cerr << "    a set of XML files in a schema suitable for information retrieval." << std::endl;
			std::cerr << "  The produced XML document files have the extension .xml and are written\n";
			std::cerr << "    into a subdirectory of <outputdir>. The subdirectories for the output are\n";
			std::cerr << "    enumerated with 4 digits in ascending order starting with 0000." << std::endl;
			std::cerr << "  Each subdirectory contains at maximum 1000 <docid>.xml output files." << std::endl;
			std::cerr << "  Each output file contains one document and has an identifier derived\n";
			std::cerr << "    from the Wikipedia title." << std::endl;
			std::cerr << "    You are encouraged to use multiple threads (option -t) for faster conversion." << std::endl;
			std::cerr << "  Besides the <docid>.xml files, the following files are written:" << std::endl;
			std::cerr << "    <docid>.err         :File with recoverable errors in the document" << std::endl;
			std::cerr << "    <docid>.mis         :File with unresolvable page links in the document" << std::endl;
			std::cerr << "    <docid>.ftl         :File with an exception thrown while processing" << std::endl;
			std::cerr << "    <docid>.wtf        :File listing some suspicious text elements\n";
			std::cerr << "                         This list is useful for tracking classification\n";
			std::cerr << "                         errors." << std::endl;
			std::cerr << "    <docid>.org        :File with a dump of the document processed\n";
			std::cerr << "                         (only written if required or on error)" << std::endl;
			std::cerr << std::endl;
			std::cerr << "Output XML format:" << std::endl;
			std::cerr << "  The tag hierarchy is as best effort intendet to be as flat as possible.\n";
			std::cerr << "  The following list explains the tags in the output:\n";
			std::cerr << "  Structural XML tags (tags marking a structure):\n";
			std::cerr << "     <quot>             :a quoted string (any type) in a document\n";
			std::cerr << "     <heading id='h#'>  :a subtitle or heading in a document\n";
			std::cerr << "     <list id='l#'>     :a list item in a document\n";
			std::cerr << "     <attr>             :an attribute in a document\n";
			std::cerr << "     <citation>         :a citation in a document\n";
			std::cerr << "     <ref>              :a reference structure in a document\n";
			std::cerr << "     <pagelink>         :a link to a page in the collection\n";
			std::cerr << "     <weblink>          :a web link to a page in the internet\n";
			std::cerr << "     <table>            :a table implementation\n";
			std::cerr << "     <tabtitle>         :sub-title text in the table\n";
			std::cerr << "     <head id='C#'>     :head cells of a table adressing a column cell\n";
			std::cerr << "     <head id='R#'>     :head cells of a table adressing a row cell\n";
			std::cerr << "     <cell id='R#'>     :data sells of a table with a list of identifiers making it addressable\n";
			std::cerr << "     <tablink           :internal link to a table in this document\n";
			std::cerr << "     <citlink>          :internal link to a citation in this document\n";
			std::cerr << "     <reflink>          :internal link to a reference in this document\n";
			std::cerr << "\n";
			std::cerr << "  Textual XML tags (tags marking a content; entities may also contain links):\n";
			std::cerr << "     <docid>            :The content specifies a unique document identifier\n";
			std::cerr << "                         (the title with '_' instead of ' ' and some other encodings)\n";
			std::cerr << "     <entity>           :A marked entity of the Wikipedia collection\n";
			std::cerr << "     <text>             :A text passage in a document\n";
			std::cerr << "     <char>             :Content is on or a sequence of special characters\n";
			std::cerr << "     <code>             :Text descibing some sort of an Id not suitable for retrival\n";
			std::cerr << "     <math>             :Text marked as LaTeX syntax math formula\n";
			std::cerr << "     <time>             :A timestimp of the form \"YYMMDDThhmmss<zone>\", <zone> = Z (UTC)\n";
			std::cerr << "     <bibref>           :bibliographic reference\n";
			std::cerr << "     <nowiki>           :information not to index\n";
			std::cerr << std::endl;
			return rt;
		}
		IStream input( argv[argi]);
		if (argi+1 < argc)
		{
			if (collectRedirects) std::cerr << "output directory ignored if option -R is specified" << std::endl;
			g_outputdir = argv[argi+1];
		}
		if (g_doTest)
		{
			if (nofThreads != 0) std::cerr << "number of threads (option -t) ignored if option --test is specified" << std::endl;
			nofThreads = 0;
		}
		if (collectRedirects)
		{
			if (nofThreads != 0) std::cerr << "number of threads (option -t) ignored if option -R is specified" << std::endl;
			if (g_beautified) std::cerr << "beautyfication (option -B) ignored if option -R is specified" << std::endl;
			if (g_dumps) std::cerr << "write dumps allways (option -D) ignored if option -R is specified" << std::endl;
			if (loadRedirects) std::cerr << "option -L not compatiple with option -R" << std::endl;
		}
		textwolf::IStreamIterator inputiterator( &input, 1<<16/*buffer size*/);
		if (nofThreads <= 0) nofThreads = 0;
		g_errorhnd = strus::createErrorBuffer_standard( NULL/*logfilehandle*/, nofThreads+2, NULL/*debugTrace*/);
		if (!g_errorhnd) throw std::runtime_error("failed to create error buffer");

		strus::local_ptr<strus::LinkMap> linkmap;
		strus::LinkMapBuilder linkmapBuilder( g_errorhnd);
		if (!linkmapfilename.empty())
		{
			linkmap.reset( new strus::LinkMap( g_errorhnd));
			if (!collectRedirects)
			{
				linkmap->load( linkmapfilename);
				g_linkmap = linkmap.get();
			}
		}

		struct WorkerArray
		{
			WorkerArray( Worker* ar_)
				:ar(ar_){}
			~WorkerArray()
			{
				delete [] ar;
			}
			Worker* ar;
		};
		WorkerArray workers( nofThreads ? new Worker[ nofThreads] : 0);
		for (int wi=0; wi < nofThreads; ++wi)
		{
			workers.ar[ wi].start( wi+1);
		}

		bool terminated = false;
		struct DocAttributes
		{
			int ns;
			std::string title;
			std::string redirect_title;
			std::string content;

			DocAttributes()
				:ns(0),title(),redirect_title(){}
			void clear()
			{
				ns = 0;
				title.clear();
				redirect_title.clear();
				content.clear();
			}
		};

		XmlScanner xs( inputiterator);
		XmlScanner::iterator itr=xs.begin(),end=xs.end();
		DocAttributes docAttributes;
		int workeridx = 0;
		int docCounter = 0;
		TagId lastTag = TagIgnored;
		std::vector<TagId> tagstack;

		for (; !terminated && itr!=end; ++itr)
		{
			if (g_verbosity >= 2) std::cout << "XML " << itr->name() << " " << strus::outputLineString( itr->content(), itr->content()+itr->size(), 80) << std::endl;
			switch (itr->type())
			{
				case XmlScanner::None: break;
				case XmlScanner::ErrorOccurred: throw std::runtime_error( itr->content());
				case XmlScanner::HeaderStart:/*no break!*/
				case XmlScanner::HeaderAttribName:/*no break!*/
				case XmlScanner::HeaderAttribValue:/*no break!*/
				case XmlScanner::HeaderEnd:/*no break!*/
				case XmlScanner::DocAttribValue:/*no break!*/
				case XmlScanner::DocAttribEnd:/*no break!*/
					break;
				case XmlScanner::TagAttribName:
				{
					if (lastTag == TagRedirect)
					{
						if (itr->size() == 5 && 0==std::memcmp( itr->content(), "title", itr->size()))
						{
							++itr;
							if (itr->type() == XmlScanner::TagAttribValue)
							{
								docAttributes.redirect_title = std::string( itr->content(), itr->size());
							}
						}
					}
					break;
				}
				case XmlScanner::TagAttribValue:
				{
					break;
				}
				case XmlScanner::OpenTag: 
				{
					lastTag = TagIgnored;
					if (namespaceset && itr->size() == 2  && 0==std::memcmp( itr->content(), "ns", itr->size()))
					{
						lastTag = TagNs;
					}
					else if (itr->size() == 4 && 0==std::memcmp( itr->content(), "page", itr->size()))
					{
						lastTag = TagPage;
						docAttributes.clear();
						if (docCounter % 1000 == 0 && !collectRedirects && !g_dumpStdout && !g_doTest)
						{
							createOutputDir( docCounter);
						}
					}
					else if (itr->size() == 5 && 0==std::memcmp( itr->content(), "title", itr->size()))
					{
						lastTag = TagTitle;
					}
					if (itr->size() == 4 && 0==std::memcmp( itr->content(), "text", itr->size()))
					{
						lastTag = TagText;
					}
					if (itr->size() == 8 && 0==std::memcmp( itr->content(), "redirect", itr->size()))
					{
						lastTag = TagRedirect;
					}
					tagstack.push_back( lastTag);
					break;
				}
				case XmlScanner::CloseTagIm:
				case XmlScanner::CloseTag:
				{
					lastTag = TagIgnored;
					TagId closedTag = TagIgnored;
					if (!tagstack.empty())
					{
						closedTag = tagstack.back();
						tagstack.pop_back();
					}
					if (closedTag == TagPage)
					{
						if (namespaceset && namespacemap.find( docAttributes.ns) == namespacemap.end())
						{
							//... ignore document but those with ns set to what is selected by option '-n'
							continue;
						}
						if (!selectDocumentPattern.empty())
						{
							std::vector<std::string>::const_iterator si = selectDocumentPattern.begin(), se = selectDocumentPattern.end();
							for (; si != se && 0==std::strstr( docAttributes.title.c_str(), si->c_str()); ++si){}
							if (si == se) continue;
						}
						if (!docAttributes.redirect_title.empty() && docAttributes.content.size() < 1000)
						{
							// ... is as Redirect
							if (collectRedirects)
							{
								++docCounter;
								std::pair<std::string,std::string> redir_parts = strus::LinkMap::getLinkParts( docAttributes.redirect_title);
								if (g_verbosity >= 1) std::cerr << strus::string_format( "%s => %s\n", docAttributes.title.c_str(), docAttributes.redirect_title.c_str());
								linkmapBuilder.redirect( docAttributes.title, redir_parts.first);

								if (counterMod && g_verbosity == 0 && docCounter % counterMod == 0)
								{
									std::cerr << "processed " << docCounter << " documents" << std::endl;
								}
							}
						}
						else if (!docAttributes.title.empty() && !docAttributes.content.empty())
						{
							// ... is as Document
							if (collectRedirects)
							{
								++docCounter;
								if (g_verbosity >= 1) std::cerr << strus::string_format( "link %s => %s\n", docAttributes.title.c_str(), docAttributes.title.c_str());
								linkmapBuilder.define( docAttributes.title);

								if (counterMod && g_verbosity == 0 && docCounter % counterMod == 0)
								{
									std::cerr << "processed " << docCounter << " documents" << std::endl;
								}
							}
							else
							{
								if (!dumpfilename.empty())
								{
									int ec = strus::writeFile( dumpfilename, docAttributes.content);
									if (ec) std::cerr << "failed to write dump file " << dumpfilename << ": " << ::strerror(ec) << std::endl;
								}
								++docCounter;
								int docIndex = docCounter-1;
								if (nofThreads)
								{
									workeridx = docIndex % nofThreads;
									workers.ar[ workeridx].push( docIndex, docAttributes.title, docAttributes.content);
								}
								else
								{
									try
									{
										Work work( docIndex, docAttributes.title, docAttributes.content, g_dumps);
										if (g_verbosity >= 1) std::cerr << strus::string_format( "process document '%s'\n", docAttributes.title.c_str()) << std::flush;
										work.process();
									} 
									catch (const std::bad_alloc&)
									{
										std::cerr << "out of memory processing document " << docAttributes.title << std::endl;
									}
									catch (const std::runtime_error& err)
									{
										std::cerr << "error processing document " << docAttributes.title << ": " << err.what() << std::endl;
									}
								}
								if (counterMod && g_verbosity == 0 && docCounter % counterMod == 0)
								{
									std::cerr << "processed " << docCounter << " documents" << std::endl;
								}
							}
						}
						else if (docAttributes.content.empty())
						{
							std::cerr << "empty document '" << docAttributes.title << "'" << std::endl;
						}
						else
						{
							std::cerr << "invalid document '" << docAttributes.title << "'" << std::endl;
						}
					}
					break;
				}
				case XmlScanner::Content:
					switch (lastTag)
					{
						case TagIgnored:
							break;
						case TagPage:
							break;
						case TagNs:
						{
							std::string contentstr( itr->content(), itr->size());
							docAttributes.ns = strus::numstring_conv::toint( contentstr, 10000);
							break;
						}
						case TagTitle:
						{
							docAttributes.title = std::string( itr->content(), itr->size());
							break;
						}
						case TagText:
						{
							docAttributes.content = std::string( itr->content(), itr->size());
							break;
						}
						case TagRedirect:
						{
							docAttributes.redirect_title = std::string( itr->content(), itr->size());
							break;
						}
					}
					break;
				case XmlScanner::Exit:
					terminated = true;
					break;
			}
		}
		for (int wi=0; wi < nofThreads; ++wi)
		{
			workers.ar[ wi].waitTermination();
		}
		if (collectRedirects && g_verbosity == 0)
		{
			std::cerr << "processed " << docCounter << " documents" << std::endl;
		}
		if (collectRedirects)
		{
			std::string unresolved_outfilename = linkmapfilename + ".mis";
			{
				linkmap.reset( new strus::LinkMap( g_errorhnd));
				if (!linkmap.get()) throw std::runtime_error("failed to create link map");
				linkmapBuilder.build( *linkmap);
				if (g_dumpStdout || g_doTest)
				{
					if (g_dumpStdout)
					{
						std::cout << "## LINKS" << std::endl;
						std::cout << std::endl;
						linkmap->write( std::cout);
					}
					else
					{
						std::ostringstream out;
						out << "## LINKS" << std::endl;
						linkmap->write( out);
						out << std::endl;
						g_testOutput.append( out.str());
					}
				}
				else
				{
					linkmap->write( linkmapfilename);
					std::cerr << "links written to " << linkmapfilename << std::endl;
				}
			}{
				std::string unresolvedstr;
				std::vector<const char*> unresolved( linkmapBuilder.unresolved());
				if (!unresolved.empty())
				{
					std::vector<const char*>::const_iterator ui = unresolved.begin(), ue = unresolved.end();
					for (; ui != ue; ++ui)
					{
						unresolvedstr.append( *ui);
						unresolvedstr.push_back( '\n');
					}
					if (g_dumpStdout || g_doTest)
					{
						if (g_dumpStdout)
						{
							std::cout << "## UNRESOLVED" << std::endl << unresolvedstr << std::endl << std::endl;
						}
						else
						{
							std::ostringstream out;
							out << "## UNRESOLVED" << std::endl << unresolvedstr << std::endl << std::endl;
							g_testOutput.append( out.str());
						}
					}
					else
					{
						int ec = strus::writeFile( unresolved_outfilename, unresolvedstr);
						if (ec)
						{
							std::cerr << "error writing unresolved links file: " << std::strerror(ec) << std::endl;
						}
						else
						{
							std::cerr << "unresolved links written to " << unresolved_outfilename << std::endl;
						}
					}
				}
			}
		}
		if (g_doTest)
		{
			std::string expected;
			int ec = strus::readFile( g_testExpectedFilename, expected);
			if (ec) throw std::runtime_error( strus::string_format( "failed to read expected file '%s' for testing (option --test <expected file>): %s", g_testExpectedFilename.c_str(), ::strerror(ec)));
			char const* ei = expected.c_str();
			char const* oi = g_testOutput.c_str();
			int line = 1;
			while (*ei && *oi)
			{
				if ((*ei == '\r' || *ei == '\n') && (*oi == '\r' || *oi == '\n'))
				{
					if (*ei == '\r') ++ei;
					if (*ei == '\n') ++ei;
					if (*oi == '\r') ++oi;
					if (*oi == '\n') ++oi;
					++line;
				}
				else if (*ei != *oi)
				{
					break;
				}
				else
				{
					++ei;
					++oi;
				}
			}
			if (*ei || *oi)
			{
				throw std::runtime_error( strus::string_format( "test outputs differ at line %d", line));
			}
		}
		if (g_errorhnd) delete g_errorhnd;
		return rt;
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << "ERROR " << e.what() << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "EXCEPTION " << e.what() << std::endl;
	}
	if (g_errorhnd) delete g_errorhnd;
	return -1;
}


