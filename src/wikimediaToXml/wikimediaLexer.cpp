/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/// \brief Lexer for Wikimedia format
/// \file wikimediaLexer.cpp
#include "wikimediaLexer.hpp"
#include "outputString.hpp"
#include "strus/base/string_conv.hpp"
#include <string>
#include <vector>
#include <utility>
#include <cstring>
#include <stdexcept>
#include <iostream>

/// \brief strus toplevel namespace
using namespace strus;


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
	return ((unsigned char)ch <= 32);
}

static bool isEqual( const char* id, const char* src, std::size_t size)
{
	std::size_t ii=0;
	for (; ii<size && (id[ii]|32)==(src[ii]|32); ++ii){}
	return ii==size;
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

static std::string parseTagContent( const char* tagname, char const*& si, const char* se)
{
	char buf[ 128];
	int tagnamelen = std::snprintf( buf, sizeof(buf), "</%s>", tagname);
	if (tagnamelen < (int)sizeof(buf))
	{
		const char* end = findPattern( si, se, buf);
		if (end)
		{
			std::string rt( si, (end - si) - tagnamelen);
			si = end;
			return rt;
		}
	}
	throw std::runtime_error( std::string("unclosed tag '") + tagname + "': " + outputString( si, se));
}

enum TagType {
	UnknwownTagType,
	TagNoWikiOpen,
	TagNoWikiClose,
	TagMathOpen,
	TagMathClose,
	TagSupOpen,
	TagSupClose,
	TagRefOpen,
	TagRefClose,
	TagSpanOpen,
	TagSpanClose,
	TagAbbrOpen,
	TagAbbrClose,
	TagCenterOpen,
	TagCenterClose,
	TagSmallOpen,
	TagSmallClose,
	TagBlockquoteOpen,
	TagBlockquoteClose,
	TagComment,
	TagBr,
};

static TagType parseTagType( char const*& si, const char* se)
{
	const char* start = si;

	if (*si != '<') throw std::runtime_error("logic error: invalid call of parseTagType");
	si++;
	if (si < se && si[0] == '!' && si[1] == '-' && si[2] == '-')
	{
		const char* end = findPattern( si+2, se, "-->");
		if (!end) throw std::runtime_error( std::string("unclosed comment tag") + ": " + outputString( start, se));
		si = end;
		return TagComment;
	}
	else
	{
		bool open = true;
		if (*si == '/')
		{
			open = false;
			++si;
		}
		const char* tgnam = si;
		while (si < se && isAlpha(*si)) ++si;
		if (si-tgnam == 6 && 0==std::memcmp( tgnam, "nowiki", si-tgnam))
		{
			char const* sn = std::strchr( si, '>');
			if (sn && sn < se)
			{
				si = (sn+1);
				return open ? TagNoWikiOpen : TagNoWikiClose;
			}
		}
		else if (si-tgnam == 4 && 0==std::memcmp( tgnam, "math", si-tgnam))
		{
			char const* sn = std::strchr( si, '>');
			if (sn && sn < se)
			{
				si = (sn+1);
				return open ? TagMathOpen : TagMathClose;
			}
		}
		else if (si-tgnam == 3 && 0==std::memcmp( tgnam, "sup", si-tgnam))
		{
			char const* sn = std::strchr( si, '>');
			if (sn && sn < se)
			{
				si = (sn+1);
				return open ? TagSupOpen : TagSupClose;
			}
		}
		else if (si-tgnam == 3 && 0==std::memcmp( tgnam, "ref", si-tgnam))
		{
			char const* sn = std::strchr( si, '>');
			if (sn && sn < se)
			{
				si = (sn+1);
				if (*(si-2) == '/') return TagComment;
				return open ? TagRefOpen : TagRefClose;
			}
		}
		else if (si-tgnam == 10 && 0==std::memcmp( tgnam, "blockquote", si-tgnam))
		{
			char const* sn = std::strchr( si, '>');
			if (sn && sn < se)
			{
				si = (sn+1);
				return open ? TagBlockquoteOpen : TagBlockquoteClose;
			}
		}
		else if (si-tgnam == 4 && 0==std::memcmp( tgnam, "span", si-tgnam))
		{
			char const* sn = std::strchr( si, '>');
			if (sn && sn < se)
			{
				si = (sn+1);
				return open ? TagSpanOpen : TagSpanClose;
			}
		}
		else if (si-tgnam == 4 && 0==std::memcmp( tgnam, "abbr", si-tgnam))
		{
			char const* sn = std::strchr( si, '>');
			if (sn && sn < se)
			{
				si = (sn+1);
				return open ? TagAbbrOpen : TagAbbrClose;
			}
		}
		else if (si-tgnam == 6 && (0==std::memcmp( tgnam, "center", si-tgnam)))
		{
			char const* sn = std::strchr( si, '>');
			if (sn && sn < se)
			{
				si = (sn+1);
				return open ? TagCenterOpen : TagCenterClose;
			}
		}
		else if (si-tgnam == 5 && 0==std::memcmp( tgnam, "small", si-tgnam))
		{
			char const* sn = std::strchr( si, '>');
			if (sn && sn < se)
			{
				si = (sn+1);
				return open ? TagSmallOpen : TagSmallClose;
			}
		}
		else if (si-tgnam == 2 && 0==std::memcmp( tgnam, "br", si-tgnam))
		{
			char const* sn = std::strchr( si, '>');
			if (sn && sn < se)
			{
				si = sn + 1;
				return TagBr;
			}
		}
	}
	si = start + 1;
	return UnknwownTagType;
}

std::string WikimediaLexer::tryParseIdentifier( char assignop)
{
	std::string rt;
	char const* ti = m_si;
	while (ti < m_se && isSpace(*ti)) ++ti;
	if (ti < m_se && isAlpha(*ti))
	{
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
	while (ti < m_se && isSpace(*ti)) ++ti;
	if (ti < m_se && *ti == assignop)
	{
		m_si = ++ti;
		return strus::string_conv::trim(rt);
	}
	else
	{
		return std::string();
	}
}

std::string WikimediaLexer::tryParseLinkId()
{
	char const* ti = m_si;
	while (ti < m_se && isSpace(*ti)) ++ti;
	const char* start = ti;
	while (ti < m_se && (isAlphaNum(*ti) || *ti == ':' || *ti == '/' || *ti == '&' || *ti == '(' || *ti == ')' || *ti == '!' || *ti == '?' || *ti == '.' || *ti == ','  || *ti == '#'|| *ti == ' ' || *ti == '-' || *ti == '_' || *ti == '\'' || *ti == '\"' || (unsigned char)*ti >= 128)) ++ti;
	const char* end = ti;
	while (ti > start && *(ti-1) == ' ') --ti;
	if (ti > start)
	{
		m_si = end;
		while (m_si < m_se && *m_si == ' ') ++m_si;
		return std::string( start, ti-start);
	}
	else
	{
		return std::string();
	}
}

std::string WikimediaLexer::tryParseURL()
{
	char const* ti = m_si;
	while (ti < m_se && isSpace(*ti)) ++ti;
	while (ti < m_se && isAlpha(*ti)) ++ti;
	if (ti+2 < m_se && ti[0] == ':' && ti[1] == '/' && ti[2] == '/')
	{
		ti += 3;
		while (ti < m_se && (!isSpace(*ti) && *ti != '|' && *ti != ']' && *ti != '[' && *ti != '}' && *ti != '{' && *ti != ')' && *ti != '(')) ++ti;
		std::string linkid( m_si, ti-m_si);
		m_si = ti;
		if (m_si < m_se && isSpace(*m_si))
		{
			while (m_si < m_se && isSpace(*m_si)) ++m_si;
			if (m_si < m_se && *m_si != '|' && *m_si != ']') --m_si;
		}
		return linkid;
	}
	return std::string();
}

bool WikimediaLexer::eatFollowChar( char expectChr)
{
	if (m_si < m_se && *m_si == expectChr)
	{
		++m_si;
		return true;
	}
	return false;
}

static char const* skipStyle_( char const* si, char const* se)
{
	int sidx=0;
	char const* start = si;
	for (;si < se && (*si == ' '||*si == '\t');++si){}
	for (;si < se && isAlpha(*si);++si,++sidx){}
	if (si < se && sidx > 0 && (*si == '=' || *si == ':'))
	{
		++si;
		if (si < se && (*si == '"' || *si == '\''))
		{
			char eb = *si;
			++si;
			for (; si < se && *si != '\n' && *si != eb; ++si){}
			if (si < se && *si == eb)
			{
				return skipStyle_( si+1, se);
			}
		}
		else if (si < se && isAlphaNum(*si))
		{
			for (;si < se && isAlphaNum(*si);++si){}
			if (si < se)
			{
				return skipStyle_( si, se);
			}
		}
	}
	return start;
}

static char const* skipStyle( char const* si, char const* se)
{
	char const* sn = skipStyle_( si, se);
	if (sn && sn != si)
	{
		si = sn;
		for (;si < se && (*si == ' '||*si == '\t');++si){}
		if (si < se && *si == '|') ++si;
	}
	return si;
}

static int countAndSkip( char const*& si, const char* se,  char ch, int maxcnt)
{
	const char* start = si;
	int rt = 0;
	for (; rt < maxcnt && si < se && ch == *si; ++si,++rt){}
	if (rt == maxcnt || rt == 0)
	{
		si = start;
		return 0;
	}
	return rt;
}

static bool compareFollowString( const char* si, const char* se, const char* str)
{
	char const* xi = str;
	for (; si < se && *xi && *xi == *si; ++si,++xi){}
	return *xi == '\0';
}

WikimediaLexem WikimediaLexer::next()
{
	m_prev_si = m_si;
	const char* start = m_si;
	try
	{
	while (m_si < m_se)
	{
		if (*m_si == '=' && m_curHeading)
		{
			int tcnt = countAndSkip( m_si, m_se, '=', 7);
			if (tcnt == m_curHeading)
			{
				m_curHeading = tcnt;
				return WikimediaLexem( WikimediaLexem::CloseHeading, tcnt, "");
			}
		}
		if (*m_si == '<')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			switch (parseTagType( m_si, m_se))
			{
				case UnknwownTagType:
					return WikimediaLexem( WikimediaLexem::Error, 0, std::string("unknown tag ") + outputLineString( m_si-1, m_se, 40));
				case TagNoWikiOpen:
					return WikimediaLexem( WikimediaLexem::NoWiki, 0, parseTagContent( "nowiki", m_si, m_se));
				case TagNoWikiClose:
					break;
				case TagMathOpen:
					return WikimediaLexem( WikimediaLexem::Math, 0, parseTagContent( "math", m_si, m_se));
				case TagMathClose:
					break;
				case TagSupOpen:
					return WikimediaLexem( WikimediaLexem::Math, 0, parseTagContent( "sup", m_si, m_se));
				case TagSupClose:
					break;
				case TagRefOpen:
					return WikimediaLexem( WikimediaLexem::OpenRef);
				case TagRefClose:
					return WikimediaLexem( WikimediaLexem::CloseRef);
				case TagSpanOpen:
					return WikimediaLexem( WikimediaLexem::OpenSpan);
				case TagSpanClose:
					return WikimediaLexem( WikimediaLexem::CloseSpan);
				case TagAbbrOpen:
					return WikimediaLexem( WikimediaLexem::OpenSpan);
				case TagAbbrClose:
					return WikimediaLexem( WikimediaLexem::CloseSpan);
				case TagCenterOpen:
					return WikimediaLexem( WikimediaLexem::OpenSpan);
				case TagCenterClose:
					return WikimediaLexem( WikimediaLexem::CloseSpan);
				case TagSmallOpen:
					return WikimediaLexem( WikimediaLexem::OpenSmall);
				case TagSmallClose:
					return WikimediaLexem( WikimediaLexem::CloseSmall);
				case TagBlockquoteOpen:
					return WikimediaLexem( WikimediaLexem::OpenBlockQuote);
				case TagBlockquoteClose:
					return WikimediaLexem( WikimediaLexem::CloseBlockQuote);
				case TagComment:
					break;
				case TagBr:
					return WikimediaLexem( WikimediaLexem::Text, 0, "\n");
			};
		}
		else if (m_si[0] == '"')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			m_si += 1;
			return WikimediaLexem( WikimediaLexem::QuotationMarker);
		}
		else if (m_si[0] == '&' && (m_se - m_si) >= 6 && compareFollowString( m_si, m_se, "&quot;"))
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			m_si += 6;
			return WikimediaLexem( WikimediaLexem::QuotationMarker);
		}
		else if (m_si[0] == '&' && (m_se - m_si) >= 6 && compareFollowString( m_si, m_se, "&nbsp;"))
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			m_si += 6;
			return WikimediaLexem( WikimediaLexem::Text, 0, " ");
		}
		else if (*m_si == '#')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			if (compareFollowString( m_si, m_se, "#REDIRECT"))
			{
				m_si += 9;
				return WikimediaLexem( WikimediaLexem::Redirect);
			}
			else
			{
				++m_si;
			}
		}
		else if (m_si[0] == '\'' && m_si[1] == '\'' && m_si[2] == '\'')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			m_si += 3;
			return WikimediaLexem( WikimediaLexem::EntityMarker);
		}
		else if (m_si[0] == '\'' && m_si[1] == '\'')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			m_si += 2;
			return WikimediaLexem( WikimediaLexem::DoubleQuoteMarker);
		}
		else if (*m_si == '[')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			++m_si;
			if (m_si >= m_se) break;

			if (*m_si == '[')
			{
				++m_si;
				if (m_si < m_se && *m_si == '#')++m_si;
				if (m_si < m_se && *m_si == ':')++m_si;
				std::string linkid = tryParseLinkId();
				if (!linkid.empty() && m_si < m_se && (*m_si == ']' || *m_si == '|'))
				{
					if (*m_si == '|') ++m_si;
					return WikimediaLexem( WikimediaLexem::OpenPageLink, 0, linkid);
				}
				else
				{
					while (m_si < m_se && *m_si != ']') ++m_si;
					if (m_si < m_se && *m_si == ']') ++m_si;
					return WikimediaLexem( WikimediaLexem::Error, 0, "illegal character in page link");
				}
			}
			else if (isAlphaNum(*m_si) || isSpace(*m_si) || *m_si == '.' || *m_si == '-' || *m_si == '+')
			{
				std::string linkid = tryParseURL();
				if (linkid.empty())
				{
					while (m_si < m_se && *m_si != ']' && (isAlphaNum(*m_si) || *m_si == '.' || *m_si == '-' || *m_si == '_' || *m_si == '-' || *m_si == '+' || isSpace(*m_si) || (unsigned char)*m_si > 128))
					{
						++m_si;
					}
					if (m_si < m_se && *m_si == ']') ++m_si;
				}
				else if (m_si < m_se && (*m_si == ']' || *m_si == '|' || *m_si == ' '))
				{
					if (*m_si == '|') ++m_si;
					return WikimediaLexem( WikimediaLexem::OpenWWWLink, 0, linkid);
				}
				else
				{
					while (m_si < m_se && *m_si != ']') ++m_si;
					if (m_si < m_se && *m_si == ']') ++m_si;
					return WikimediaLexem( WikimediaLexem::Error, 0, "illegal character in WWW link");
				}
			}
			else
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, " [");
			}
		}
		else if (*m_si == '{')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			++m_si;
			if (m_si >= m_se) break;

			if (*m_si == '{')
			{
				++m_si;
				const char* start = m_si;
				while (m_si < m_se && *m_si != '}' && *m_si != '|' && (isAlphaNum(*m_si) || *m_si == '-' || *m_si == '_' || *m_si == '-' || isSpace(*m_si)))
				{
					++m_si;
				}
				if (m_si < m_se && (*m_si == '}' || *m_si == '|'))
				{
					std::string citid( strus::string_conv::trim( std::string( start, m_si - start)));
					if (*m_si == '|') ++m_si;
					return WikimediaLexem( WikimediaLexem::OpenCitation, 0, citid);
				}
				else
				{
					m_si = start;
					return WikimediaLexem( WikimediaLexem::OpenCitation);
				}
			}
			else if (*m_si == '|')
			{
				++m_si;
				m_si = skipStyle( m_si, m_se);
				return WikimediaLexem( WikimediaLexem::OpenTable);
			}
		}
		else if (*m_si == '|')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			++m_si;
			if (m_si == m_se) break;

			if (*m_si == '|')
			{
				++m_si;
				return WikimediaLexem( WikimediaLexem::DoubleColDelim);
			}
			else
			{
				std::string name = tryParseIdentifier( '=');
				return WikimediaLexem( WikimediaLexem::ColDelim, 0, name);
			}
		}
		else if (*m_si == '!' && m_si+1 < m_se && m_si[1] == '!')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			m_si += 2;
			m_si = skipStyle( m_si, m_se);
			return WikimediaLexem( WikimediaLexem::TableHeadDelim);
		}
		else if (*m_si == '\n')
		{
			m_curHeading = 0;
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			while (m_si < m_se && isSpace(*m_si)) ++m_si;
			if (m_si == m_se)
			{
				return WikimediaLexem( WikimediaLexem::EndOfLine);
			}
			else if (*m_si == '!')
			{
				++m_si;
				m_si = skipStyle( m_si, m_se);
				return WikimediaLexem( WikimediaLexem::TableHeadDelim);
			}
			else if (*m_si == '|')
			{
				++m_si;
				if (m_si >= m_se) break;

				if (*m_si == '-')
				{
					++m_si;
					m_si = skipStyle( m_si, m_se);
					return WikimediaLexem( WikimediaLexem::TableRowDelim);
				}
				else if (*m_si == '+')
				{
					++m_si;
					m_si = skipStyle( m_si, m_se);
					return WikimediaLexem( WikimediaLexem::TableTitle);
				}
				else if (*m_si == '}')
				{
					++m_si;
					return WikimediaLexem( WikimediaLexem::CloseTable);
				}
				else if (*m_si == '|')
				{
					++m_si;
					return WikimediaLexem( WikimediaLexem::DoubleColDelim);
				}
				else
				{
					std::string name = tryParseIdentifier( '=');
					return WikimediaLexem( WikimediaLexem::TableColDelim);
				}
			}
			else if (*m_si == '*')
			{
				int lidx = countAndSkip( m_si, m_se, '*', 5);
				if (lidx > 1)
				{
					return WikimediaLexem( WikimediaLexem::ListItem, lidx, "");
				}
				++m_si;
			}
			else if (*m_si == '=')
			{
				int tcnt = countAndSkip( m_si, m_se, '=', 7);
				if (tcnt > 1)
				{
					m_curHeading = tcnt;
					return WikimediaLexem( WikimediaLexem::OpenHeading, tcnt-1, "");
				}
			}
			else if (m_si+6 < m_se && (isEqual("File:",m_si,5) || isEqual("Image:",m_si,6)))
			{
				m_si += 6;
				for (; m_si < m_se && *m_si != '\n' && *m_si != '|'; ++m_si){}
				if (m_si < m_se && *m_si == '|')
				{
					++m_si;
					return WikimediaLexem( WikimediaLexem::ListItem, 1, "");
				}
			}
			else
			{
				return WikimediaLexem( WikimediaLexem::EndOfLine);
			}
		}
		else if (*m_si == '}' && m_si+1 < m_se && m_si[1] == '}')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			m_si += 2;
			return WikimediaLexem( WikimediaLexem::CloseCitation);
		}
		else if (*m_si == ']')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			++m_si;
			if (m_si < m_se && *m_si == ']')
			{
				++m_si;
				return WikimediaLexem( WikimediaLexem::ClosePageLink);
			}
			return WikimediaLexem( WikimediaLexem::CloseWWWLink, 0, "]");
		}
		else if (m_si + 2 < m_se && m_si[0] == ':' && m_si[1] == '/' && m_si[2] == '/')
		{
			const char* si_bk = m_si;
			char const* ti = m_si;
			while (m_si - ti < 6 &&  ti > start && isAlpha(*(ti-1))) --ti;
			if (m_si - ti >= 3 && m_si - ti < 6 && ti >= start)
			{
				if (ti == start)
				{
					m_si = ti;
					std::string url = tryParseURL();
					if (!url.empty())
					{
						return WikimediaLexem( WikimediaLexem::Url, 0, url);
					}
					else
					{
						m_si = si_bk;
						m_si += 3;
					}
				}
				else
				{
					m_si = ti;
					return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
				}
			}
			else
			{
				m_si += 3;
			}
		}
		else
		{
			++m_si;
		}
	}
	}
	catch (const std::runtime_error& err)
	{
		return WikimediaLexem( WikimediaLexem::Error, 0, err.what());
	}
	if (start != m_si)
	{
		return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
	}
	else
	{
		return WikimediaLexem( WikimediaLexem::EoF);
	}
}

std::string WikimediaLexer::currentSourceExtract() const
{
	return outputString( m_prev_si, m_si+20);
}

std::string WikimediaLexer::rest() const
{
	return std::string( m_si, m_se-m_si);
}


