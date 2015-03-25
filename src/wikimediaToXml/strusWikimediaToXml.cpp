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

#define STRUS_LOWLEVEL_DEBUG

typedef textwolf::XMLPrinter<textwolf::charset::UTF8,textwolf::charset::UTF8,std::string> XmlPrinter;
typedef textwolf::XMLScanner<textwolf::IStreamIterator,textwolf::charset::UTF8,textwolf::charset::UTF8,std::string> XmlScanner;

static std::string outputString( const char* si, const char* se)
{
	if (!si || !se)
	{
		throw std::runtime_error( "outputString called with NULL argument");
	}
	else if (se - si > 60)
	{
		return std::string( si, 30) + "..." + std::string( se-30, 30);
	}
	else
	{
		return std::string( si, se-si);
	}
}

static bool isAlpha( char ch)
{
	return (ch|32) >= 'a' && (ch|32) <= 'z';
}

static bool isDigit( char ch)
{
	return (ch) >= '0' && (ch) <= '9';
}

static bool isAlphaNum( char ch)
{
	return isAlpha(ch) || isDigit(ch);
}

static bool isSpace( char ch)
{
	return (ch <= 32);
}

static std::string trim( const std::string& src)
{
	char const* si = src.c_str();
	const char* se = src.c_str() + src.size();
	while (si < se && isSpace(*si)) ++si;
	while (si < se && isSpace(*(se-1))) --se;
	return std::string( si, (std::size_t)(se-si));
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

static const char* skipToEndTag( const char* tagname, char const* si, const char* se)
{
	std::size_t tagnamelen = std::strlen( tagname);
	const char* end = findPattern( si+1, se, "</");
	if (!end || 0!=std::memcmp( tagname, end, tagnamelen))
	{
		std::cerr << "WARNING unclosed tag '" << tagname << "': " << outputString( si, se) << std::endl;
		return si+tagnamelen;
	}
	else
	{
		return end;
	}
}

static const char* skipTag( char const* si, const char* se)
{
#ifdef STRUS_LOWLEVEL_DEBUG
	std::cerr << "SKIP TAG '" << outputString( si, se) << "'" << std::endl;
#endif
	const char* start = si++;
	if (si < se && si[0] == '!' && si[1] == '-' && si[2] == '-')
	{
		const char* end = findPattern( si+2, se, "-->");
		if (!end)
		{
			std::cerr << "WARNING comment not closed:" << outputString( start, se) << std::endl;
			return si+3;
		}
		else
		{
			return end;
		}
	}
	else
	{
		const char* tgnam = si;
		if (!isAlpha(*si) && *si != '/') return si;
		for (; si < se && *si != '>'; ++si){}
		if (0==std::memcmp( tgnam, "nowiki", si-tgnam))
		{
			return skipToEndTag( "nowiki", si, se);
		}
		else if (0==std::memcmp( tgnam, "math", si-tgnam))
		{
			return skipToEndTag( "math", si, se);
		}
		else
		{
			return si+1;
		}
	}
}

static const char* skipLine( char const* si, const char* se)
{
	const char* eoln =(const char*)std::memchr( si, '\n', se-si);
	if (eoln) return eoln+1;
	return se;
}

class Lexer
{
public:
	enum LexemId
	{
		LexemEOF,
		LexemText,
		LexemRedirect,
		LexemHeading1,
		LexemHeading2,
		LexemHeading3,
		LexemHeading4,
		LexemHeading5,
		LexemHeading6,
		LexemOpenCitation,
		LexemCloseCitation,
		LexemOpenWWWLink,
		LexemCloseSquareBracket,
		LexemOpenLink,
		LexemCloseLink,
		LexemOpenTable,
		LexemCloseTable,
		LexemTableRowDelim,
		LexemTableTitle,
		LexemTableHeadDelim,
		LexemColDelim
	};
	static const char* lexemIdName( LexemId lexemId)
	{
		static const char* ar[] =
		{
			"EOF","Text","Redirect","Heading1","Heading2","Heading3",
			"Heading4","Heading5","Heading6","OpenCitation","CloseCitation",
			"OpenWWWLink", "CloseSquareBracket","OpenLink","CloseLink",
			"OpenTable", "CloseTable","TableRowDelim","TableTitle",
			"TableHeadDelim","ColDelim"
		};
		return ar[lexemId];
	}

	struct Lexem
	{
		Lexem( LexemId id_, const std::string& value_)
			:id(id_),value(value_){}
		Lexem( LexemId id_)
			:id(id_),value(){}
		Lexem( const Lexem& o)
			:id(o.id),value(o.value){}

		LexemId id;
		std::string value;
	};

	Lexer( const char* src, std::size_t size)
		:m_si(src),m_se(src+size){}

	Lexem next();
	std::string tryParseIdentifier( char assignop);
	std::string tryParseURL();

private:
	char const* m_si;
	const char* m_se;
};


std::string Lexer::tryParseIdentifier( char assignop)
{
	std::string rt;
	char const* ti = m_si;
	while (ti < m_se && isSpace(*ti)) ++ti;
	if (ti < m_se && isAlpha(*ti))
	{
		++ti;
		while (ti < m_se && (isAlphaNum(*ti) || *ti == '-' || isSpace(*ti)))
		{
			char ch = *ti++;
			if (isAlpha(ch))
			{
				rt.push_back( ch|32);
			}
			else if (ch == '-')
			{
				rt.push_back( '-');
			}
			else if (rt.size()>0 && rt[ rt.size()-1] != ' ')
			{
				rt.push_back( ' ');
			}
		}
	}
	if (ti < m_se && *ti == assignop)
	{
		while (isSpace( rt[rt.size()-1]))
		{
			rt.resize( rt.size()-1);
		}
		m_si = ++ti;
		return rt;
	}
	else
	{
		return std::string();
	}
}

std::string Lexer::tryParseURL()
{
	char const* ti = m_si;
	while (ti < m_se && isSpace(*ti)) ++ti;
	while (ti < m_se && isAlpha(*ti)) ++ti;
	if (ti+2 < m_se && ti[0] == ':' && ti[1] == '/' && ti[2] == '/')
	{
		ti += 3;
		while (ti < m_se && (!isSpace(*ti) && *ti != ']')) ++ti;
		std::string linkid( m_si, ti-m_si);
		if (ti < m_se && isSpace(*ti))
		{
			m_si = ti+1;
		}
		else
		{
			m_si = ti;
		}
		return linkid;
	}
	return std::string();
}

Lexer::Lexem Lexer::next()
{
	const char* start = m_si;
	while (m_si < m_se)
	{
		if (*m_si == '<')
		{
			m_si = skipTag( m_si, m_se);
			start = m_si;
		}
		else if (*m_si == '#')
		{
			if (0==std::memcmp( "#REDIRECT", m_si, 9))
			{
				m_si += 9;
				return Lexem( LexemRedirect);
			}
		}
		else if (*m_si == '[')
		{
			if (start != m_si)
			{
				return Lexem( LexemText, std::string( start, m_si - start));
			}
			++m_si;
			if (m_si >= m_se) break;

			if (*m_si == '[')
			{
				++m_si;
				std::string name = tryParseIdentifier( ':');
				return Lexem( LexemOpenLink, name);
			}
			else if (isAlpha(*m_si) || isSpace(*m_si))
			{
				std::string linkid = tryParseURL();
				if (linkid.size())
				return Lexem( LexemOpenWWWLink, linkid);
			}
		}
		else if (*m_si == '{')
		{
			if (start != m_si)
			{
				return Lexem( LexemText, std::string( start, m_si - start));
			}
			++m_si;
			if (m_si >= m_se) break;

			if (*m_si == '{')
			{
				++m_si;
				std::string name = tryParseIdentifier( '|');
				return Lexem( LexemOpenCitation, name);
			}
			else if (*m_si == '|')
			{
				return Lexem( LexemOpenTable);
			}
		}
		else if (*m_si == '|')
		{
			if (start != m_si)
			{
				return Lexem( LexemText, std::string( start, m_si - start));
			}
			++m_si;
			if (m_si >= m_se) break;

			if (*m_si == '-')
			{
				++m_si;
				return Lexem( LexemTableRowDelim);
			}
			else if (*m_si == '+')
			{
				++m_si;
				const char* eoln = skipLine( m_si, m_se);
				return Lexem( LexemTableTitle, (eoln == m_se)?std::string(m_si,m_se-m_si):std::string(m_si,m_se-m_si-1));
			}
			else if (*m_si == '}')
			{
				++m_si;
				return Lexem( LexemCloseTable);
			}
			else
			{
				return Lexem( LexemColDelim);
			}
		}
		else if (*m_si == '!' && m_si+1 < m_se && m_si[1] == '!')
		{
			m_si += 2;
			return Lexem( LexemTableHeadDelim);
		}
		else if (*m_si == '\n')
		{
			while (m_si < m_se && isSpace(*m_si)) ++m_si;
			if (m_si < m_se)
			{
				if (*m_si == '!')
				{
					++m_si;
					return Lexem( LexemTableHeadDelim);
				}
				else if (*m_si == '=')
				{
					char const* ti = ++m_si;
					for (; ti != m_se && *ti == '='; ++ti){}
					if (ti-m_si <= 5 && ti-m_si >= 1)
					{
						const char* oldsrc = m_si;
						m_si = ti;
						std::string name = tryParseIdentifier('=');
						if (name.size())
						{
							for (; m_si != m_se && *m_si == '='; ++m_si){}
							return Lexem( (LexemId)(LexemHeading1+(ti-m_si)), name);
						}
						else
						{
							m_si = oldsrc;
						}
					}
				}
			}
		}
		else if (*m_si == '}' && m_si+1 < m_se && m_si[1] == '}')
		{
			m_si += 2;
			return Lexem( LexemCloseCitation);
		}
		else if (*m_si == ']')
		{
			++m_si;
			if (m_si < m_se && *m_si == ']')
			{
				return Lexem( LexemCloseLink);
			}
			else
			{
				return Lexem( LexemCloseSquareBracket);
			}
		}
		++m_si;
	}
	if (start != m_si)
	{
		return Lexem( LexemText, std::string( start, m_si - start));
	}
	else
	{
		return Lexem( LexemEOF);
	}
}

enum StateId
{
	StateText,
	StateCitation,
	StateLink,
	StateWWWLink,
	StateTable
};
enum TableStateId
{
	TableStateOpen,
	TableStateRow,
	TableStateCol
};
static const char* stateIdName( StateId i)
{
	static const char* ar[] = {"Text","Citation","Link","WWWLink","Table"};
	return ar[i];
}
struct State
{
	State( StateId id_)
		:id(id_)
	{
		substate.table = TableStateOpen;
	}
	State( const State& o)
		:id(o.id),substate(o.substate){}
	StateId id;
	union 
	{
		TableStateId table;
	} substate;
};

static void processDataRowCol( const char* tag, std::string& res, const std::string& data, XmlPrinter& xmlprinter)
{
	Lexer lexer( data.c_str(), data.size());
	std::string id = lexer.tryParseIdentifier('=');
	std::string val = trim( id);
	if (id.size())
	{
		if (xmlprinter.state() == XmlPrinter::TagElement)
		{
			xmlprinter.printAttribute( id.c_str(), id.size(), res);
			xmlprinter.printValue( val.c_str(), val.size(), res);
		}
		else
		{
			xmlprinter.printOpenTag( id.c_str(), id.size(), res);
			xmlprinter.printValue( val.c_str(), val.size(), res);
			xmlprinter.printCloseTag( res);
		}
	}
	else if (tag && tag[0])
	{
		xmlprinter.printOpenTag( tag, std::strlen(tag), res);
		xmlprinter.printValue( val.c_str(), val.size(), res);
		xmlprinter.printCloseTag( res);
	}
	else
	{
		xmlprinter.printValue( "", 0, res);
		xmlprinter.printValue( val.c_str(), val.size(), res);
	}
}

static void processOpenLink( std::string& res, const std::string& tagnam, Lexer& lexer, XmlPrinter& xmlprinter)
{
	xmlprinter.printOpenTag( "link", 4, res);
	if (tagnam.size())
	{
		xmlprinter.printAttribute( "type", 4, res);
		xmlprinter.printValue( tagnam.c_str(), tagnam.size(), res);
	}
	std::string databuf;
	for (Lexer::Lexem lexem = lexer.next(); lexem.id == Lexer::LexemText; lexem = lexer.next())
	{
		xmlprinter.printValue( "", 0, databuf);
		xmlprinter.printValue( tagnam.c_str(), tagnam.size(), databuf);
	}
	processDataRowCol( "id", res, databuf, xmlprinter);
}

static void processOpenCitation( std::string& res, const std::string& tagnam, Lexer& lexer, XmlPrinter& xmlprinter)
{
	xmlprinter.printOpenTag( "cit", 3, res);
	xmlprinter.printAttribute( "type", 4, res);
	if (tagnam.size())
	{
		xmlprinter.printValue( tagnam.c_str(), tagnam.size(), res);
	}
	else
	{
		xmlprinter.printValue( "page", 4, res);
	}
}

static void processOpenWWWLink( std::string& res, const std::string& tagnam, Lexer& lexer, XmlPrinter& xmlprinter)
{
	xmlprinter.printOpenTag( "wwwlink", 7, res);
	if (tagnam.size())
	{
		xmlprinter.printAttribute( "id", 2, res);
		xmlprinter.printValue( tagnam.c_str(), tagnam.size(), res);
	}
}

static std::string processWikimediaText( const char* src, std::size_t size, XmlPrinter& xmlprinter)
{
	bool done = false;
	std::vector<State> stack;
	stack.push_back( StateText);
	std::string databuf;
	bool redirectTagOpen = false;
	static const char* headingTagName[] = {"h1","h2","h3","h4","h5","h6"};

	std::string rt;
	Lexer lexer(src,size);

	for (Lexer::Lexem lexem = lexer.next(); !done; lexem = lexer.next())
	{
AGAIN:
		if (xmlprinter.lasterror())
		{
			throw std::runtime_error( std::string( "textwolf xmlprinter: ") + xmlprinter.lasterror());
		}
		switch (stack.back().id)
		{
			case StateText:
				switch (lexem.id)
				{
					case Lexer::LexemEOF:
						if (redirectTagOpen)
						{
							xmlprinter.printCloseTag( rt);
							redirectTagOpen = false;
						}
						done = true;
						break;
					case Lexer::LexemText:
						xmlprinter.printValue( "", 0, rt);
						xmlprinter.printValue( lexem.value.c_str(), lexem.value.size(), rt);
						break;
					case Lexer::LexemRedirect:
						if (stack.size() > 1 || redirectTagOpen)
						{
							std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						}
						else
						{
							xmlprinter.printOpenTag( "redirect", 8, rt);
							redirectTagOpen = true;
						}
						break;
					case Lexer::LexemHeading1:
					case Lexer::LexemHeading2:
					case Lexer::LexemHeading3:
					case Lexer::LexemHeading4:
					case Lexer::LexemHeading5:
					case Lexer::LexemHeading6:
						xmlprinter.printOpenTag( headingTagName[lexem.id-Lexer::LexemHeading1], 2, rt);
						xmlprinter.printValue( lexem.value.c_str(), lexem.value.size(), rt);
						xmlprinter.printCloseTag( rt);
						break;
					case Lexer::LexemOpenCitation:
						processOpenCitation( rt, lexem.value, lexer, xmlprinter);
						stack.push_back( StateCitation);
						break;
					case Lexer::LexemCloseCitation:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						break;
					case Lexer::LexemOpenWWWLink:
						processOpenWWWLink( rt, lexem.value, lexer, xmlprinter);
						stack.push_back( StateWWWLink);
						break;
					case Lexer::LexemCloseSquareBracket:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						break;
					case Lexer::LexemOpenLink:
						processOpenLink( rt, lexem.value, lexer, xmlprinter);
						stack.push_back( StateLink);
						goto AGAIN;
					case Lexer::LexemCloseLink:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						break;
					case Lexer::LexemOpenTable:
						xmlprinter.printOpenTag( "table", 5, rt);
						stack.push_back( StateTable);
						break;
					case Lexer::LexemCloseTable:/*no break here!*/
					case Lexer::LexemTableRowDelim:/*no break here!*/
					case Lexer::LexemTableTitle:/*no break here!*/
					case Lexer::LexemTableHeadDelim:/*no break here!*/
					case Lexer::LexemColDelim:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						break;
				}
				break;
			case StateCitation:
				if (lexem.id != Lexer::LexemText)
				{
					processDataRowCol( "", rt, databuf, xmlprinter);
					databuf.clear();
				}
				switch (lexem.id)
				{
					case Lexer::LexemEOF:
						if (redirectTagOpen)
						{
							xmlprinter.printCloseTag( rt);
							redirectTagOpen = false;
						}
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						done = true;
						break;
					case Lexer::LexemText:
						xmlprinter.printValue( "", 0, databuf);
						xmlprinter.printValue( lexem.value.c_str(), lexem.value.size(), databuf);
						break;
					case Lexer::LexemRedirect:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						break;
					case Lexer::LexemHeading1:
					case Lexer::LexemHeading2:
					case Lexer::LexemHeading3:
					case Lexer::LexemHeading4:
					case Lexer::LexemHeading5:
					case Lexer::LexemHeading6:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						xmlprinter.printCloseTag( rt);
						stack.pop_back();
						goto AGAIN;
					case Lexer::LexemOpenCitation:
						xmlprinter.printCloseTag( rt);
						stack.pop_back();
						goto AGAIN;
					case Lexer::LexemCloseCitation:
						xmlprinter.printCloseTag( rt);
						stack.pop_back();
						break;
					case Lexer::LexemOpenWWWLink:
						processOpenWWWLink( rt, lexem.value, lexer, xmlprinter);
						stack.push_back( StateWWWLink);
						break;
					case Lexer::LexemCloseSquareBracket:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						break;
					case Lexer::LexemOpenLink:
						processOpenLink( rt, lexem.value, lexer, xmlprinter);
						stack.push_back( StateLink);
						goto AGAIN;
					case Lexer::LexemCloseLink:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						xmlprinter.printCloseTag( rt);
						stack.pop_back();
						goto AGAIN;
					case Lexer::LexemOpenTable:
						xmlprinter.printOpenTag( "table", 5, rt);
						stack.push_back( StateTable);
						break;
					case Lexer::LexemCloseTable:/*no break here!*/
					case Lexer::LexemTableRowDelim:/*no break here!*/
					case Lexer::LexemTableTitle:/*no break here!*/
					case Lexer::LexemTableHeadDelim:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						xmlprinter.printCloseTag( rt);
						stack.pop_back();
						goto AGAIN;
						break;
					case Lexer::LexemColDelim:
						break;
				}
				break;
			case StateLink:
				if (lexem.id != Lexer::LexemText)
				{
					processDataRowCol( "", rt, databuf, xmlprinter);
					databuf.clear();
				}
				switch (lexem.id)
				{
					case Lexer::LexemEOF:
						if (redirectTagOpen)
						{
							xmlprinter.printCloseTag( rt);
							redirectTagOpen = false;
						}
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						done = true;
						break;
					case Lexer::LexemText:
						xmlprinter.printValue( "", 0, databuf);
						xmlprinter.printValue( lexem.value.c_str(), lexem.value.size(), databuf);
						break;
					case Lexer::LexemRedirect:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						break;
					case Lexer::LexemHeading1:
					case Lexer::LexemHeading2:
					case Lexer::LexemHeading3:
					case Lexer::LexemHeading4:
					case Lexer::LexemHeading5:
					case Lexer::LexemHeading6:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						xmlprinter.printCloseTag( rt);
						stack.pop_back();
						goto AGAIN;
					case Lexer::LexemOpenCitation:
						processOpenCitation( rt, lexem.value, lexer, xmlprinter);
						stack.push_back( StateCitation);
						break;
					case Lexer::LexemCloseCitation:
						xmlprinter.printCloseTag( rt);
						stack.pop_back();
						goto AGAIN;
					case Lexer::LexemOpenWWWLink:
						processOpenWWWLink( rt, lexem.value, lexer, xmlprinter);
						stack.push_back( StateWWWLink);
						break;
					case Lexer::LexemCloseSquareBracket:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						xmlprinter.printCloseTag( rt);
						stack.pop_back();
						goto AGAIN;
					case Lexer::LexemOpenLink:
						xmlprinter.printCloseTag( rt);
						stack.pop_back();
						goto AGAIN;
					case Lexer::LexemCloseLink:
						xmlprinter.printCloseTag( rt);
						stack.pop_back();
						break;
					case Lexer::LexemOpenTable:
						xmlprinter.printOpenTag( "table", 5, rt);
						stack.push_back( StateTable);
						break;
					case Lexer::LexemCloseTable:/*no break here!*/
					case Lexer::LexemTableRowDelim:/*no break here!*/
					case Lexer::LexemTableTitle:/*no break here!*/
					case Lexer::LexemTableHeadDelim:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						xmlprinter.printCloseTag( rt);
						stack.pop_back();
						goto AGAIN;
						break;
					case Lexer::LexemColDelim:
						break;
				}
				break;
			case StateWWWLink:
				if (lexem.id != Lexer::LexemText)
				{
					processDataRowCol( "", rt, databuf, xmlprinter);
					databuf.clear();
				}
				switch (lexem.id)
				{
					case Lexer::LexemEOF:
						if (redirectTagOpen)
						{
							xmlprinter.printCloseTag( rt);
							redirectTagOpen = false;
						}
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						done = true;
						break;
					case Lexer::LexemText:
						xmlprinter.printValue( "", 0, databuf);
						xmlprinter.printValue( lexem.value.c_str(), lexem.value.size(), databuf);
						break;
					case Lexer::LexemRedirect:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						break;
					case Lexer::LexemHeading1:
					case Lexer::LexemHeading2:
					case Lexer::LexemHeading3:
					case Lexer::LexemHeading4:
					case Lexer::LexemHeading5:
					case Lexer::LexemHeading6:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						xmlprinter.printCloseTag( rt);
						stack.pop_back();
						goto AGAIN;
					case Lexer::LexemOpenCitation:
						processOpenCitation( rt, lexem.value, lexer, xmlprinter);
						stack.push_back( StateCitation);
						break;
					case Lexer::LexemCloseCitation:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						xmlprinter.printCloseTag( rt);
						stack.pop_back();
						goto AGAIN;
					case Lexer::LexemOpenWWWLink:
						std::cerr << "WARNING nested links, unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						xmlprinter.printCloseTag( rt);
						stack.pop_back();
						goto AGAIN;
					case Lexer::LexemCloseSquareBracket:
						xmlprinter.printCloseTag( rt);
						stack.pop_back();
						break;
					case Lexer::LexemOpenLink:
						processOpenLink( rt, lexem.value, lexer, xmlprinter);
						stack.push_back( StateLink);
						break;
					case Lexer::LexemCloseLink:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						xmlprinter.printCloseTag( rt);
						stack.pop_back();
						goto AGAIN;
					case Lexer::LexemOpenTable:
						xmlprinter.printOpenTag( "table", 5, rt);
						stack.push_back( StateTable);
						break;
					case Lexer::LexemCloseTable:/*no break here!*/
					case Lexer::LexemTableRowDelim:/*no break here!*/
					case Lexer::LexemTableTitle:/*no break here!*/
					case Lexer::LexemTableHeadDelim:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						xmlprinter.printCloseTag( rt);
						stack.pop_back();
						goto AGAIN;
						break;
					case Lexer::LexemColDelim:
						break;
				}
				break;
			case StateTable:
				if (lexem.id != Lexer::LexemText)
				{
					processDataRowCol( "i", rt, databuf, xmlprinter);
					databuf.clear();
				}
				switch (lexem.id)
				{
					case Lexer::LexemEOF:
						switch (stack.back().substate.table)
						{
							case TableStateCol:
								xmlprinter.printCloseTag( rt);
								/*no break here!*/
							case TableStateRow:
								xmlprinter.printCloseTag( rt);
								stack.back().substate.table = TableStateOpen;
								/*no break here!*/
							case TableStateOpen:
								break;
						}
						xmlprinter.printCloseTag( rt);
						stack.pop_back();
						if (redirectTagOpen)
						{
							xmlprinter.printCloseTag( rt);
							redirectTagOpen = false;
						}
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						done = true;
						break;
					case Lexer::LexemText:
						xmlprinter.printValue( "", 0, databuf);
						xmlprinter.printValue( lexem.value.c_str(), lexem.value.size(), databuf);
						break;
					case Lexer::LexemRedirect:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						break;
					case Lexer::LexemHeading1:
					case Lexer::LexemHeading2:
					case Lexer::LexemHeading3:
					case Lexer::LexemHeading4:
					case Lexer::LexemHeading5:
					case Lexer::LexemHeading6:
						std::cerr << "WARNING table not closed, unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						xmlprinter.printCloseTag( rt);
						stack.pop_back();
						goto AGAIN;
					case Lexer::LexemOpenCitation:
						processOpenCitation( rt, lexem.value, lexer, xmlprinter);
						stack.push_back( StateCitation);
						break;
					case Lexer::LexemCloseCitation:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						break;
					case Lexer::LexemOpenWWWLink:
						processOpenWWWLink( rt, lexem.value, lexer, xmlprinter);
						stack.push_back( StateWWWLink);
						break;
					case Lexer::LexemCloseSquareBracket:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						break;
					case Lexer::LexemOpenLink:
						processOpenLink( rt, lexem.value, lexer, xmlprinter);
						stack.push_back( StateLink);
						break;
					case Lexer::LexemCloseLink:
						std::cerr << "WARNING unexpected lexem " << Lexer::lexemIdName(lexem.id) << " in " << stateIdName(stack.back().id) << std::endl;
						break;
					case Lexer::LexemOpenTable:
						xmlprinter.printOpenTag( "table", 5, rt);
						stack.push_back( StateTable);
						break;
					case Lexer::LexemCloseTable:
						switch (stack.back().substate.table)
						{
							case TableStateCol:
								xmlprinter.printCloseTag( rt);
								/*no break here!*/
							case TableStateRow:
								xmlprinter.printCloseTag( rt);
								stack.back().substate.table = TableStateOpen;
								/*no break here!*/
							case TableStateOpen:
								break;
						}
						xmlprinter.printCloseTag( rt);
						stack.pop_back();
						break;
					case Lexer::LexemTableRowDelim:
						switch (stack.back().substate.table)
						{
							case TableStateCol:
								xmlprinter.printCloseTag( rt);
								stack.back().substate.table = TableStateRow;
								/*no break here!*/
							case TableStateRow:
								xmlprinter.printCloseTag( rt);
								xmlprinter.printOpenTag( "tr", 2, rt);
								break;
							case TableStateOpen:
								xmlprinter.printOpenTag( "tr", 2, rt);
								stack.back().substate.table = TableStateRow;
								break;
						}
						break;
					case Lexer::LexemTableTitle:
						xmlprinter.printOpenTag( "title", 5, rt);
						xmlprinter.printValue( lexem.value.c_str(), lexem.value.size(), rt);
						xmlprinter.printCloseTag( rt);
						break;
					case Lexer::LexemTableHeadDelim:
						switch (stack.back().substate.table)
						{
							case TableStateCol:
								xmlprinter.printCloseTag( rt);
								xmlprinter.printOpenTag( "th", 2, rt);
								break;
							case TableStateRow:
								xmlprinter.printCloseTag( rt);
								xmlprinter.printOpenTag( "tr", 2, rt);
								xmlprinter.printOpenTag( "th", 2, rt);
								stack.back().substate.table = TableStateCol;
								break;
							case TableStateOpen:
								xmlprinter.printOpenTag( "tr", 2, rt);
								xmlprinter.printOpenTag( "th", 2, rt);
								stack.back().substate.table = TableStateCol;
								break;
						}
						break;
					case Lexer::LexemColDelim:
						switch (stack.back().substate.table)
						{
							case TableStateCol:
								xmlprinter.printCloseTag( rt);
								xmlprinter.printOpenTag( "td", 2, rt);
								break;
							case TableStateRow:
								xmlprinter.printCloseTag( rt);
								xmlprinter.printOpenTag( "tr", 2, rt);
								xmlprinter.printOpenTag( "td", 2, rt);
								stack.back().substate.table = TableStateCol;
								break;
							case TableStateOpen:
								xmlprinter.printOpenTag( "tr", 2, rt);
								xmlprinter.printOpenTag( "td", 2, rt);
								stack.back().substate.table = TableStateCol;
								break;
						}
						break;
				}
				break;
		}
	}
	if (stack.size() > 1)
	{
		std::cerr << "WARNING unexpected end of data" << std::endl;
	}
	return rt;
}


static void processText( std::ostream& out, char const* si, std::size_t size)
{
	XmlPrinter xmlprinter( true);
	out << processWikimediaText( si, size, xmlprinter);
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
			if (xmlprinter.lasterror())
			{
				throw std::runtime_error( std::string( "textwolf xmlprinter: ") + xmlprinter.lasterror());
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


