/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "textwolf/istreamiterator.hpp"
#include "textwolf/xmlscanner.hpp"
#include "textwolf/xmlprinter.hpp"
#include "textwolf/charset_utf8.hpp"
#include "strus/base/inputStream.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <string>
#include <stdexcept>
#include <memory>
#include <vector>
#include <cstdio>

#undef STRUS_LOWLEVEL_DEBUG

typedef textwolf::XMLPrinter<textwolf::charset::UTF8,textwolf::charset::UTF8,std::string> XmlPrinter;
typedef textwolf::XMLScanner<textwolf::IStreamIterator,textwolf::charset::UTF8,textwolf::charset::UTF8,std::string> XmlScanner;

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

int main( int argc, const char* argv[])
{
	int rt = 0;
	try
	{
		if (argc <= 1 || argc > 2)
		{
			if (argc > 2) std::cerr << "too many arguments" << std::endl;
			std::cerr << "Usage: validateXml <inputfile>" << std::endl;
			std::cerr << "<inputfile>   :File to process or '-' for stdin" << std::endl;
			return 0;
		}
		IStream input( argv[1]);
		textwolf::IStreamIterator inputiterator( &input, 1<<12/*buffer size*/);

		XmlScanner xs( inputiterator);
		XmlScanner::iterator itr=xs.begin(),end=xs.end();

		unsigned int nofmb_printed = 0;
		std::vector<std::string> tagstack;

		for (; itr!=end; itr++)
		{
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << textwolf::XMLScannerBase::getElementTypeName( itr->type()) << ": " << std::string( itr->content(), itr->size()) << std::endl;
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
					break;
				}
				case XmlScanner::TagAttribName:
				{
					break;
				}
				case XmlScanner::TagAttribValue:
				{
					break;
				}
				case XmlScanner::OpenTag: 
				{
					tagstack.push_back( std::string( itr->content(), itr->size()));
					if (itr->size() == 4 && 0==std::memcmp( itr->content(), "text", itr->size()))
					{
						unsigned int nofmb = xs.getIterator().position() / 1000000;
						if (nofmb > nofmb_printed)
						{
							nofmb_printed = nofmb;
							printf( "\rprocessed %d MB of data", nofmb);
						}
					}
					break;
				}
				case XmlScanner::CloseTagIm:
					if (tagstack.empty())
					{
						throw std::runtime_error( "tags not balanced");
					}
					tagstack.pop_back();
					break;
				case XmlScanner::CloseTag:
				{
					if (tagstack.empty())
					{
						throw std::runtime_error( "tags not balanced");
					}
					if (tagstack.back().size() != itr->size()
					||  0!=std::memcmp( itr->content(), tagstack.back().c_str(), itr->size()))
					{
						throw std::runtime_error(std::string("tags not balanced: ") + std::string( itr->content(), itr->size()) + " != " + std::string( tagstack.back().c_str(), tagstack.back().size()));
					}
					tagstack.pop_back();
					break;
				}
				case XmlScanner::Content:
				{
					break;
				}
				case XmlScanner::Exit: break;
			}
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


