/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/

#include "textwolf/istreamiterator.hpp"
#include "textwolf/xmlscanner.hpp"
#include "textwolf/xmlprinter.hpp"
#include "textwolf/charset_utf8.hpp"
#include "inputStream.hpp"
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
		strus::InputStream input( argv[1]);
		textwolf::StdInputStream inputreader( input.stream());
		textwolf::IStreamIterator inputiterator( &inputreader);

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


