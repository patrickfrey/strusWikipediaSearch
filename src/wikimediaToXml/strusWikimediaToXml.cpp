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
#include <map>
#include <stdint.h>

typedef textwolf::XMLPrinter<textwolf::charset::UTF8,textwolf::charset::UTF8,std::string> XmlPrinter;
typedef textwolf::XMLScanner<textwolf::IStreamIterator,textwolf::charset::UTF8,textwolf::charset::UTF8,std::string> XmlScanner;

static bool isAlphaNum( char ch)
{
	return ((ch|32) >= 'a' && (ch|32) <= 'z') || (ch >= '0' && ch <= '9');
}

static bool isSpace( char ch)
{
	return (ch <= 32);
}

static const char* findPattern( char const* si, const char* se, const char* pattern)
{
	int plen = std::strlen( pattern);
	for (;;)
	{
		const char* rt = (const char*)std::memchr( si, pattern[0], se-si);
		if (!rt) return 0;
		if (se - rt >= plen && 0==std::memcmp( rt, pattern, plen))
		{
			return rt + plen;
		}
		si = rt+1;
	}
}

static const char* skipTag( char const* si, const char* se)
{
	const char* start = si++;
	if (si < se && si[0] == '!' && si[1] == '-' && si[2] == '-')
	{
		const char* end = findPattern( si+2, se, "-->");
		if (!end)
		{
			std::cerr << "WARNING comment not closed in table:" << std::string( start, 30) << std::endl;
		}
		return end;
	}
	else if (isAlphaNum(*si))
	{
		std::string pattern("</");
		while (si < se && isAlphaNum(*si))
		{
			pattern.push_back( *si);
		}
		pattern.push_back( '>');
		const char* end = findPattern( si+2, se, pattern.c_str());
		return end;
	}
	else
	{
		return si;
	}
}

enum {MaxWWWLinkSize=256,MaxPageLinkSize=4000};
static const char* skipLink( char const* si, const char* se)
{
	const char* start = si;
	char sb = *si++;
	bool dup = false;
	int maxlinksize = MaxWWWLinkSize;
	if (si < se && *si == sb)
	{
		dup = true;
		maxlinksize = MaxPageLinkSize;
		++si;
	}
	char eb = 0;
	if (sb == '[') eb = ']';
	if (sb == '{') eb = '}';
	if (!eb)
	{
		throw std::logic_error( "illegal call of skipLink");
	}
	while (si < se && si - start < maxlinksize)
	{
		if (*si == eb)
		{
			++si;
			if (dup)
			{
				if (*si == eb) return si+1;
			}
			else
			{
				return si;
			}
		}
		else if (*si == '{' || *si == '[')
		{
			si = skipLink( si, se);
		}
		else
		{
			++si;
		}
	}
	if (dup)
	{
		std::cerr << "WARNING skip link did not find end: " << std::string(start,20) << std::endl;
	}
	else
	{
		//[+]std::cerr << "WARNING skip link did not find end: " << std::string(start,20) << std::endl;
		si = start;
		while (si < se && !isSpace(*si))
		{
			++si;
		}
	}
	return si;
}

static const char* skipTable( char const* si, const char* se)
{
	const char* start = si;
	char sb = *si++;
	char eb = 0;
	if (sb == '{') eb = '}';
	if (!eb || si > se || *si != '|') throw std::logic_error( "illegal call of skipTable");
	++si;
	while (si+1 < se)
	{
		if (si[0] == '|' && si[1] == eb)
		{
			return si+2;
		}
		else if (*si == '{')
		{
			if (si[1] == '|')
			{
				si = skipTable( si, se);
			}
			else
			{
				si = skipLink( si, se);
			}
		}
		else if (*si == '[')
		{
			si = skipLink( si, se);
		}
		else
		{
			++si;
		}
	}
	std::cerr << "WARNING skip table did not find end: " << std::string(start,20) << std::endl;
	return si;
}

static const char* skipLine( char const* si, const char* se)
{
	const char* eoln =(const char*)std::memchr( si, '\n', se-si);
	if (eoln) return eoln+1;
	return se;
}

#if 0 //UNUSED BUT WILL BE USED
static const char* skipSpaces( char const* si, const char* se)
{
	for (; si != se && isSpace(*si); ++si){}
	return si;
}
#endif

static const char* skipIdent( char const* si, const char* se)
{
	for (; si != se && isAlphaNum(*si); ++si){}
	return si;
}

static std::string parseIdent( char const*& si, const char* se)
{
	std::string rt;
	for (; si != se && isAlphaNum(*si); ++si)
	{
		rt.push_back( *si | 32);
	}
	return rt;
}

#if 0 //UNUSED BUT WILL BE USED
static std::string parseValue( char const*& si, const char* se)
{
	si = skipSpaces( si, se);
	if (si != se && (*si == '\'' || *si == '\"'))
	{
		char eb = *si++;
		const char* start = si;
		while (si < se)
		{
			if (*si == eb)
			{
				++si;
				return std::string( start, si-start);
			}
			else if (*si == '{' || *si == '[')
			{
				si = skipLink( si, se);
			}
			else
			{
				++si;
			}
		}
		std::cerr << "WARNING parse string value did not find end: " << std::string(start,20) << std::endl;
		return std::string( start, si-start);
	}
	else if (*si == '{' || *si == '[')
	{
		si = skipLink( si, se);
		return std::string( start, si-start);
	}
	else
	{
		const char* start = si;
		while (!isSpace(*si)) ++si;
		return std::string( start, si-start);
	}
}
#endif

static std::string mapText( char const* si, std::size_t size);

struct ValueRow
{
	ValueRow()
		:items(),attributes(){}
	ValueRow( const ValueRow& o)
		:items(o.items),attributes(o.attributes){}

	std::vector<std::string> items;
	std::map<std::string,std::string> attributes;
};

static ValueRow parseValueRow( char const*& si, const char* se, char elemdelim, char enddelim)
{
	ValueRow rt;
	bool found = false;
	const char* start = si;
	while (si < se && !found)
	{
		char const* ti = si;
		std::string name = parseIdent( ti, se);
		if (*ti == '=')
		{
			if (name.empty())
			{
				std::cerr << "WARNING empty row name:"
						<< std::string(si,20) << "..." << std::endl;
			}
			si = ti+1;

			const char* valstart = si;
			for (; si < se && *si != enddelim && *si != elemdelim; ++si)
			{
				if (*si == '{' || *si == '[')
				{
					si = skipLink( si, se);
					--si;
				}
			}
			rt.attributes[ name] = std::string( valstart, si-valstart);
		}
		else
		{
			char const* start = si;
			while (si < se && *si != enddelim && *si != elemdelim)
			{
				if (*si == '{' || *si == '[')
				{
					si = skipLink( si, se);
				}
				else
				{
					++si;
				}
			}
			rt.items.push_back( std::string( start, si-start));
		}
		if (si < se)
		{
			found = (*si++ == enddelim);
		}
	}
	if (!found)
	{
		std::cerr << "WARNING unterminated value row" << std::string(start,20) << std::endl;
	}
	return rt;
}

static void printValueRowAttributes( XmlPrinter& xmlprinter, std::string& buf, const ValueRow& row, const char* valuetag)
{
	std::map<std::string,std::string>::const_iterator
		ai = row.attributes.begin(), ae = row.attributes.end();
	for (; ai != ae; ++ai)
	{
		xmlprinter.printAttribute( ai->first.c_str(), ai->first.size(), buf);
		xmlprinter.printValue( ai->second.c_str(), ai->second.size(), buf);
	}
	std::vector<std::string>::const_iterator
		vi = row.items.begin(), ve = row.items.end();
	for (int vidx=0; vi != ve; ++vi,++vidx)
	{
		if (valuetag)
		{
			xmlprinter.printOpenTag( valuetag, std::strlen(valuetag), buf);
			xmlprinter.printValue( "", 0, buf);
			buf.append( mapText( vi->c_str(), vi->size()));
			xmlprinter.printCloseTag( buf);
		}
		else
		{
			xmlprinter.printValue( "", 0, buf);
			buf.append( mapText( vi->c_str(), vi->size()));
		}
	}
}

static void processText( std::ostream& out, char const* si, std::size_t size)
{
	XmlPrinter xmlprinter( true);
	const char* se = si + size;
	while (si < se)
	{
		if (*si == '[')
		{
			++si;
			if (si != se && *si == '[')
			{
				//... WikiLink
				++si;
				if (si == se)
				{
					std::cerr << "WARNING unexpected end of link" << std::endl;
					break;
				}
				const char* ti = si;
				std::string type = parseIdent( si, se);
				if (*si == ':')
				{
					++si;
				}
				else
				{
					type = "page";
					si = ti;
				}
				ValueRow row = parseValueRow( si, se, '|', ']');

				std::string buf;
				xmlprinter.printOpenTag( "wikilink", 8, buf);
				xmlprinter.printAttribute( "type", 4, buf);
				xmlprinter.printValue( type.c_str(), type.size(), buf);

				if (row.items.size() == 1 && row.attributes.size() == 0)
				{
					xmlprinter.printAttribute( "id", 2, buf);
					xmlprinter.printValue( row.items[0].c_str(), row.items[0].size(), buf);
					xmlprinter.printValue( row.items[0].c_str(), row.items[0].size(), buf);
				}
				else if (row.items.size() == 2 && row.attributes.size() == 0)
				{
					xmlprinter.printAttribute( "id", 2, buf);
					xmlprinter.printValue( row.items[0].c_str(), row.items[0].size(), buf);
					xmlprinter.printValue( row.items[1].c_str(), row.items[1].size(), buf);
				}
				else
				{
					printValueRowAttributes( xmlprinter, buf, row, 0);
				}
				xmlprinter.printCloseTag( buf);
				out << buf;
			}
			else if (isAlphaNum(*si))
			{
				const char* idstart = si;
				si = skipIdent( si, se);
				if (*si == ':')
				{
					//... WWWLink
					for (++si;(unsigned char)*si > 32 && *si != ']'; ++si)
					{
						if (*si == '[' || *si == '{')
						{
							si = skipLink( si, se);
							--si;
						}
					}
					const char* idend = si;
					const char* linktext = 0;
					if (*si == ' ')
					{
						linktext = ++si;
						for (++si;(unsigned char)*si >= 32 && *si != ']'; ++si)
						{
							if (*si == '[' || *si == '{')
							{
								si = skipLink( si, se);
								--si;
							}
						}
					}
					if (*si != ']')
					{
						//[+]std::cerr << "WARNING unterminated WWW link: " << std::string( idstart, si-idstart) << std::endl;
						linktext = 0;
					}
					++si;
					std::string buf;
					xmlprinter.printOpenTag( "wwwlink", 7, buf);
					xmlprinter.printAttribute( "id", 2, buf);
					xmlprinter.printValue( idstart, idend-idstart, buf);
					if (linktext)
					{
						xmlprinter.printValue( "", 0, buf);
						buf.append( mapText( linktext, si - linktext));
					}
					xmlprinter.printCloseTag( buf);
					out << buf;
				}
				else
				{
					si = idstart;
					out << '[' << *si++;
				}
			}
		}
		else if (*si == '{')
		{
			++si;
			if (si != se && *si == '{')
			{
				//... Citation
				++si;
				std::string type = parseIdent( si, se);
				while (si < se && (isSpace(*si) || *si == '-'))
				{
					type.push_back( ' ');
					++si;
					type.append( parseIdent( si, se));
				}
				ValueRow row = parseValueRow( si, se, '|', '}');

				std::string buf;
				xmlprinter.printOpenTag( "cit", 3, buf);
				xmlprinter.printAttribute( "type", 4, buf);
				xmlprinter.printValue( type.c_str(), type.size(), buf);

				printValueRowAttributes( xmlprinter, buf, row, "i");
				xmlprinter.printCloseTag( buf);

				if (si < se && *si == '}')
				{
					++si;
				}
				else
				{
					std::cerr << "WARNING unexpected character in citation:" << std::string(si,20) << "..." << std::endl;
				}
			}
			else if (*si == '|')
			{
				if (si+1 < se && si[1] == '}')
				{
					// ... catch '{|}'
					++si;
					continue;
				}
				//...Embedded table
				const char* start = si-1; 
				const char* tnext = skipTable( start, se);
				const char* tend = tnext-2;
				++si;

				ValueRow row = parseValueRow( si, tend, '|', '\n');

				std::string buf;
				xmlprinter.printOpenTag( "table", 5, buf);
				printValueRowAttributes( xmlprinter, buf, row, "i");

				while (si < tend)
				{
					const char* tag = 0;
					if (*si == '*')
					{
						tag = "td";
						row = parseValueRow( si, tend, '*', '\n');
					}
					else if (*si == '|')
					{
						if (si < tend && si[1] == '+')
						{
							const char* eoln = skipLine( si, tend);
							xmlprinter.printOpenTag( "title", 5, buf);
							xmlprinter.printValue( "", 0, buf);

							buf.append( mapText( si+2, eoln-si));
							xmlprinter.printCloseTag( buf);
							si = eoln;
							continue;
						}
						tag = "td";
						row = parseValueRow( si, tend, '|', '\n');
					}
					else if (*si == '!')
					{
						tag = "th";
						row = parseValueRow( si, tend, '!', '\n');
					}
					else if (*si == '<')
					{
						const char* endtag = skipTag( si, tend);
						if (!endtag)
						{
							std::cerr << "WARNING tag not closed in table:" << std::string( start, 30) << std::endl;
						}
						si = endtag;
						continue;
					}
					else
					{
						std::cerr << "WARNING unknown tag charater in table: " << std::string(si,30) << std::endl;
						si = skipLine( si, tend);
						continue;
					}
					xmlprinter.printOpenTag( "tr", 2, buf);
					printValueRowAttributes( xmlprinter, buf, row, tag);
					xmlprinter.printCloseTag( buf);
				}
				si = tnext;
				xmlprinter.printCloseTag( buf);//...close table
			}
			else
			{
				out << '{' << *si++;
			}
		}
		else if (*si == '#')
		{
			++si;
			if (si+8 < se && 0==std::memcmp( si,"REDIRECT",8))
			{
				std::string buf;
				xmlprinter.printOpenTag( "label", 5, buf);
				xmlprinter.printAttribute( "id", 2, buf);
				xmlprinter.printValue( "redirect", 8, buf);
				xmlprinter.printValue( "", 0, buf);
				buf.append( mapText( si, se-si));
				processText( out, si, se-si);
				xmlprinter.printCloseTag( buf);
				si = se;
			}
			else
			{
				out << '#' << *si++;
			}
		}
		else if (*si == '=')
		{
			char const* ti = si+1;
			for (; ti != se && *ti == '='; ++ti){}
			if (ti-si <= 6 && ti-si >= 2)
			{
				static const char* tagnam[] = {"h1","h2","h3","h4","h5","h6"};
				std::size_t level = ti - si - 1;
				char const* te = skipLine( ti, se);
				if (*(te-1) == '=')
				{
					si = te;
					while (*(te-1) == '=') --te;
					std::string buf;
					xmlprinter.printOpenTag( tagnam[level], 2, buf);
					xmlprinter.printValue( ti, te-ti, buf);
					xmlprinter.printCloseTag( buf);
					out << buf;
				}
				else
				{
					out << '=' << *si++;
				}
			}
			else
			{
				out << '=' << *si++;
			}
		}
		else
		{
			out << *si++;
		}
	}
}

static std::string mapText( char const* src, std::size_t size)
{
	std::ostringstream rt;
	processText( rt, src, size);
	return rt.str();
}


int main( int argc, const char* argv[])
{
	int rt = 0;
	try
	{
		if (argc <= 1 || argc > 2)
		{
			if (argc > 2) std::cerr << "too many arguments" << std::endl;
			std::cerr << "Usage: strusWikimediaToXml <inputfile>" << std::endl;
			std::cerr << "<inputfile>   :File to process or '-' for stdin" << std::endl;
			return 0;
		}
		strus::InputStream input( argv[1]);

		XmlScanner xs( input.stream());
		XmlScanner::iterator itr=xs.begin(),end=xs.end();
		XmlPrinter xmlprinter;

		std::string buf;
		xmlprinter.printHeader( 0, 0, buf);
		std::cout << buf;
		for (; itr!=end; itr++)
		{
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
					std::cerr << "WARNING unexpected element '" << itr->name() << "'" << std::endl;
				}
				case XmlScanner::TagAttribName:
				{
					buf.clear();
					xmlprinter.printAttribute( itr->content(), itr->size(), buf);
					std::cout << buf;
					break;
				}
				case XmlScanner::TagAttribValue:
				{
					buf.clear();
					xmlprinter.printValue( itr->content(), itr->size(), buf);
					std::cout << buf;
					break;
				}
				case XmlScanner::OpenTag: 
				{
					buf.clear();
					xmlprinter.printOpenTag( itr->content(), itr->size(), buf);
					std::cout << buf;
					break;
				}
				case XmlScanner::CloseTagIm:
				case XmlScanner::CloseTag:
				{
					buf.clear();
					xmlprinter.printCloseTag( buf);
					std::cout << buf;
					break;
				}
				case XmlScanner::Content:
				{
					buf.clear();
					xmlprinter.printValue( "", 0, buf);
					std::cout << buf;
					processText( std::cout, itr->content(), itr->size());
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


