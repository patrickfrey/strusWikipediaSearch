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
#include "strus/base/local_ptr.hpp"
#include "strus/base/fileio.hpp"
#include "strus/base/thread.hpp"
#include "strus/base/numstring.hpp"
#include "strus/base/inputStream.hpp"
#include "strus/base/string_format.hpp"
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

#define STRUS_LOWLEVEL_DEBUG

static bool g_silent = false;

typedef textwolf::XMLScanner<textwolf::IStreamIterator,textwolf::charset::UTF8,textwolf::charset::UTF8,std::string> XmlScanner;

static std::string parseItemText( strus::WikimediaLexer& lexer)
{
	std::string rt;
	for (strus::WikimediaLexem lexem = lexer.next(); lexem.id == strus::WikimediaLexem::Text; lexem = lexer.next())
	{
		rt.append( lexem.value);
	}
	lexer.unget();
	return rt;
}

static void parseDocumentText( strus::DocumentStructure& doc, const char* src, std::size_t size)
{
	strus::WikimediaLexer lexer(src,size);

	for (strus::WikimediaLexem lexem = lexer.next(); lexem.id != strus::WikimediaLexem::EoF; lexem = lexer.next())
	{
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "LEXEM " << strus::WikimediaLexem::idName( lexem.id) << " '" << strus::outputString( lexem.value.c_str(), lexem.value.c_str() + lexem.value.size()) << "'" << std::endl;
#endif
		switch (lexem.id)
		{
			case strus::WikimediaLexem::EoF:
				break;
			case strus::WikimediaLexem::Text:
				doc.addText( lexem.value);
				break;
			case strus::WikimediaLexem::Redirect:
				throw std::runtime_error("unexpected redirect in document");
				break;
			case strus::WikimediaLexem::ListItem1:
			case strus::WikimediaLexem::ListItem2:
			case strus::WikimediaLexem::ListItem3:
			case strus::WikimediaLexem::ListItem4:
				doc.openListItem( (int)lexem.id - (int)strus::WikimediaLexem::ListItem1 + 1);
				break;
			case strus::WikimediaLexem::EndOfLine:
				doc.closeOpenListItem();
				break;
			case strus::WikimediaLexem::Heading1:
			case strus::WikimediaLexem::Heading2:
			case strus::WikimediaLexem::Heading3:
			case strus::WikimediaLexem::Heading4:
			case strus::WikimediaLexem::Heading5:
			case strus::WikimediaLexem::Heading6:
				doc.closeOpenListItem();
				doc.addHeading( (int)lexem.id - (int)strus::WikimediaLexem::Heading1 + 1, lexem.value);
				break;
			case strus::WikimediaLexem::OpenCitation:
				doc.openCitation();
				break;
			case strus::WikimediaLexem::CloseCitation:
				doc.closeCitation();
				break;
			case strus::WikimediaLexem::OpenWWWLink:
				doc.addWebLink( lexem.value, parseItemText( lexer));
				break;
			case strus::WikimediaLexem::OpenLink:
			{
				doc.addPageLink( lexem.value, parseItemText( lexer));
				break;
			}
			case strus::WikimediaLexem::CloseLink:
				break;
			case strus::WikimediaLexem::OpenTable:
			{
				std::string title = parseItemText( lexer);
				std::vector<std::string> colnames;

				for (strus::WikimediaLexem lexem = lexer.next(); lexem.id == strus::WikimediaLexem::TableHeadDelim; lexem = lexer.next())
				{
					colnames.push_back( parseItemText( lexer));
				}
				doc.openTable( title, colnames);
				break;
			}
			case strus::WikimediaLexem::CloseTable:
				doc.closeTable();
				break;
			case strus::WikimediaLexem::TableRowDelim:
				doc.addTableRow();
				break;
			case strus::WikimediaLexem::TableTitle:
				doc.addError("unexpected table title");
				break;
			case strus::WikimediaLexem::TableHeadDelim:
				doc.addError("unexpected table head delimiter");
				break;
			case strus::WikimediaLexem::ColDelim:
				doc.addTableCol( parseItemText( lexer));
				break;
		}
	}
}

static void outputFile( int fileCounter, const strus::DocumentStructure& doc)
{
	char dirnam[ 16];
	std::snprintf( dirnam, sizeof(dirnam), "%03u", fileCounter / 10000);
	std::string filename( strus::joinFilePath( dirnam, doc.id() + "*.xml"));
	int ec = strus::writeFile( filename, doc.toxml());
	if (ec) std::cerr << "error writing file " << filename << ": " << std::strerror(ec) << std::endl;
}

class Work
{
public:
	Work()
		:m_fileindex(-1),m_title(),m_content(){}
	Work( int fileindex_, const std::string& title_, const std::string& content_)
		:m_fileindex(fileindex_),m_title(title_),m_content(content_){}
	Work( const Work& o)
		:m_fileindex(o.m_fileindex),m_title(o.m_title),m_content(o.m_content){}

	bool empty() const
	{
		return m_content.empty();
	}
	int fileindex() const				{return m_fileindex;}
	const std::string& title() const		{return m_title;}
	const std::string& content() const		{return m_content;}
	void process()
	{
		strus::DocumentStructure doc;
		doc.setTitle( m_title);
		parseDocumentText( doc, m_content.c_str(), m_content.size());
		doc.finish();
		if (!g_silent)
		{
			std::vector<std::string>::const_iterator ei = doc.errors().begin(), ee = doc.errors().end();
			for (int eidx=1; ei != ee; ++ei,++eidx)
			{
				std::cerr << "error " << eidx << " processing document '" << m_title << "':" << *ei << std::endl;
			}
		}
		outputFile( m_fileindex, doc);
	}

private:
	int m_fileindex;
	std::string m_title;
	std::string m_content;
};

class Worker
{
public:
	Worker()
		:m_thread(0),m_terminated(false),m_eof(true){}
	~Worker()
	{
		waitTermination();
	}

	void push( int filecounter, const std::string& title, const std::string& content)
	{
		strus::unique_lock lock( m_queue_mutex);
		m_queue.push( Work( filecounter, title, content));
		m_cv.notify_one();
	}
	void terminate()
	{
		strus::unique_lock lock( m_queue_mutex);
		m_terminated = true;
		m_cv.notify_all();
	}
	void waitTermination()
	{
		if (m_thread)
		{
			strus::unique_lock lock( m_queue_mutex);
			m_eof = true;
			m_cv.notify_all();
			m_thread->join();
			delete m_thread;
			m_thread = 0;
		}
	}
	bool waitSignal()
	{
		if (m_terminated) return false;
		strus::unique_lock lock( m_cv_mutex);
		m_cv.wait( lock);
		return !m_terminated;
	}
	Work fetch()
	{
		Work rt;
		strus::unique_lock lock( m_queue_mutex);
		if (m_queue.empty())
		{
			if (m_eof) m_terminated = true;
		}
		else
		{
			rt = m_queue.front();
			m_queue.pop();
		}
		return rt;
	}

	void run()
	{
		while (waitSignal())
		{
			std::string title;
			do
			{
				try
				{
					title.clear();
					Work work( fetch());
					if (!work.title().empty())
					{
						title = work.title();
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
			while (!title.empty());
		}
	}
	void start()
	{
		if (m_thread) throw std::runtime_error("start called twice");
		m_thread = new strus::thread( &Worker::run, this);
	}

private:
	strus::condition_variable m_cv;
	strus::mutex m_cv_mutex;
	strus::mutex m_queue_mutex;
	std::queue<Work> m_queue;
	strus::thread* m_thread;
	bool m_terminated;
	bool m_eof;
};

class IStream
	:public textwolf::IStream
{
public:
	explicit IStream( const std::string& docpath)
		:m_impl(docpath){}
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

static std::string getTagContentString( XmlScanner::iterator& itr, const XmlScanner::iterator& end)
{
	++itr;
	if (itr == end) return std::string();
	if (itr->type() == XmlScanner::Content)
	{
		return std::string( itr->content(), itr->size());
	}
	else
	{
		return std::string();
	}
}

static int getTagContentInt( XmlScanner::iterator& itr, const XmlScanner::iterator& end, int maxval)
{
	return strus::numstring_conv::toint( getTagContentString( itr, end), maxval);
}

static int getIntOptionArg( int argi, int argc, const char* argv[])
{
	if (argv[argi+1])
	{
		return strus::numstring_conv::toint( argv[argi+1], std::numeric_limits<int>::max());
	}
	else
	{
		throw std::runtime_error( std::string("no argument given for option ") + argv[argi]);
	}
}

int main( int argc, const char* argv[])
{
	int rt = 0;
	try
	{
		int argi = 1;
		int nofThreads = 0;
		std::set<int> namespacemap;
		bool namespaceset = false;
		bool printusage = false;
		bool collectRedirects = false;

		for (;argi < argc; ++argi)
		{
			if (0==std::strcmp(argv[argi],"-s"))
			{
				g_silent = true;
			}
			else if (0==std::strcmp(argv[argi],"-h"))
			{
				printusage = true;
			}
			else if (0==std::strcmp(argv[argi],"-R"))
			{
				collectRedirects = true;
			}
			else if (0==std::memcmp(argv[argi],"-n",2))
			{
				namespaceset = true;
				namespacemap.insert( getIntOptionArg( argi, argc, argv));
				++argi;
			}
			else if (0==std::memcmp(argv[argi],"-t",2))
			{
				nofThreads = getIntOptionArg( argi, argc, argv);
				++argi;
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
		if (argc > argi+2 || argc < argi+2)
		{
			if (argc > argi+2) std::cerr << "too many arguments" << std::endl;
			if (argc < argi+2) std::cerr << "too few arguments" << std::endl;
			printusage = true;
			rt = -1;
		}
		if (printusage)
		{
			std::cerr << "Usage: strusWikimediaToXml [options] <inputfile> <outputdir>" << std::endl;
			std::cerr << "<inputfile>   :File to process or '-' for stdin" << std::endl;
			std::cerr << "options:" << std::endl;
			std::cerr << "    -h           :print this usage" << std::endl;
			std::cerr << "    -s           :silent mode (suppress warnings)" << std::endl;
			std::cerr << "    -t <threads> :number of threads to use is <threads>" << std::endl;
			std::cerr << "    -n <ns>      :reduce output to namespace <ns> (0=article)" << std::endl;
			std::cerr << "    -R           :collect redirects only" << std::endl;
			return rt;
		}
		IStream input( argv[argi]);
		textwolf::IStreamIterator inputiterator( &input, 1<<16/*buffer size*/);
		if (nofThreads <= 0) nofThreads = 0;
		struct WorkerArray
		{
			WorkerArray( Worker* ar_)
				:ar(ar_){}
			~WorkerArray()
			{
				delete ar;
			}
			Worker* ar;
		};
		WorkerArray workers( nofThreads ? new Worker[ nofThreads] : 0);
		for (int wi=0; wi < nofThreads; ++wi)
		{
			workers.ar[ wi].start();
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
			}
		};

		XmlScanner xs( inputiterator);
		XmlScanner::iterator itr=xs.begin(),end=xs.end();
		DocAttributes docAttributes;
		int taglevel = 0;
		int workeridx = 0;
		int docCounter = 0;
		enum TagId {TagIgnored,TagPage,TagNs,TagTitle,TagText,TagRedirect};
		TagId lastTag = TagIgnored;

		for (; !terminated && itr!=end; ++itr)
		{
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "element " << itr->name() << " '" << std::string( itr->content(), itr->size()) << "'" << std::endl;
#endif
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
				{
					if (!g_silent)
					{
						std::cerr << "unexpected element '" << itr->name() << "'" << std::endl;
					}
				}
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
						docAttributes.ns = getTagContentInt( itr, end, 10000);
					}
					else if (itr->size() == 4 && 0==std::memcmp( itr->content(), "page", itr->size()))
					{
						lastTag = TagPage;
						taglevel = 1;
						docAttributes.clear();
					}
					else if (itr->size() == 5 && 0==std::memcmp( itr->content(), "title", itr->size()))
					{
						lastTag = TagTitle;
						docAttributes.title = getTagContentString( itr, end);
						if (docAttributes.title.empty())
						{
							throw std::runtime_error( "empty title document");
						}
					}
					if (itr->size() == 4 && 0==std::memcmp( itr->content(), "text", itr->size()))
					{
						lastTag = TagText;
						docAttributes.content = getTagContentString( itr, end);
						if (docAttributes.content.empty())
						{
							throw std::runtime_error( strus::string_format( "empty content document '%s'", docAttributes.title.c_str()));
						}
					}
					if (itr->size() == 8 && 0==std::memcmp( itr->content(), "redirect", itr->size()))
					{
						lastTag = TagRedirect;
					}
					break;
				}
				case XmlScanner::CloseTagIm:
				case XmlScanner::CloseTag:
				{
					lastTag = TagIgnored;
					--taglevel;
					if (taglevel == 0)
					{
						if (!docAttributes.redirect_title.empty() && docAttributes.content.size() < 1000)
						{
							if (collectRedirects)
							{
								std::cout << strus::string_format( "%s\t%s\n", docAttributes.title.c_str(), docAttributes.redirect_title.c_str());
							}
						}
						else if (!docAttributes.title.empty() || !docAttributes.content.empty())
						{
							if (!collectRedirects)
							{
								if (nofThreads)
								{
									workeridx = docCounter % nofThreads;
									workers.ar[ workeridx].push( docCounter, docAttributes.title, docAttributes.content);
									++docCounter;
									if (docCounter % 1000 == 0)
									{
										std::cerr << "processed " << docCounter << " documents" << std::endl;
									}
								}
								else
								{
									try
									{
										Work work( docCounter, docAttributes.title, docAttributes.content);
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
									++docCounter;
									if (docCounter % 1000 == 0)
									{
										std::cerr << "processed " << docCounter << " documents" << std::endl;
									}
								}
							}
						}
					}
					break;
				}
				case XmlScanner::Content:
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
	return -1;
}


