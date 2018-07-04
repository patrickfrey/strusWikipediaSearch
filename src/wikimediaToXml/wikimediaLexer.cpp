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
#include "strus/base/utf8.hpp"
#include <string>
#include <vector>
#include <utility>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <cstdio>

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
	TagCodeOpen,
	TagCodeClose,
	TagTtOpen,
	TagTtClose,
	TagSourceOpen,
	TagSourceClose,
	TagMathOpen,
	TagMathClose,
	TagCiteOpen,
	TagCiteClose,
	TagChemOpen,
	TagChemClose,
	TagSupOpen,
	TagSupClose,
	TagSubOpen,
	TagSubClose,
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
	TagBigOpen,
	TagBigClose,
	TagUOpen,
	TagUClose,
	TagLiOpen,
	TagLiClose,
	TagSOpen,
	TagSClose,
	TagGalleryOpen,
	TagGalleryClose,
	TagBlockquoteOpen,
	TagBlockquoteClose,
	TagPoemOpen,
	TagPoemClose,
	TagDivOpen,
	TagDivClose,
	TagComment,
	TagBr
};

static bool tryParseTag( const char* tagnam, char const*& si, const char* se)
{
	const char* start = si;
	int ti = 0;
	for (;si < se && tagnam[ti] && tagnam[ti] == (*si|32); ++si,++ti){}
	if (!tagnam[ti] && !isAlphaNum(*si))
	{
		char const* sn = std::strchr( si, '>');
		if (sn && sn < se)
		{
			si = sn+1;
			return true;
		}
	}
	si = start;
	return false;
}

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
		char const* sn = std::strchr( si, '>');
		if (sn && sn < se)
		{
			if (*(sn-1) == '/')
			{
				si = sn+1;
				return TagComment;
				//.... immediate close tags not encolsing any content are considered unimportant for text retrieval and thus marked as comments.
			}
		}
		if (tryParseTag( "nowiki", si, se)) return open ? TagNoWikiOpen : TagNoWikiClose;
		else if (tryParseTag( "code", si, se)) return open ? TagCodeOpen : TagCodeClose;
		else if (tryParseTag( "tt", si, se)) return open ? TagTtOpen : TagTtClose;
		else if (tryParseTag( "source", si, se)) return open ? TagSourceOpen : TagSourceClose;
		else if (tryParseTag( "math", si, se)) return open ? TagMathOpen : TagMathClose;
		else if (tryParseTag( "chem", si, se)) return open ? TagChemOpen : TagChemClose;
		else if (tryParseTag( "sup", si, se)) return open ? TagSupOpen : TagSupClose;
		else if (tryParseTag( "sub", si, se)) return open ? TagSubOpen : TagSubClose;
		else if (tryParseTag( "ref", si, se)) return open ? TagRefOpen : TagRefClose;
		else if (tryParseTag( "blockquote", si, se)) return open ? TagBlockquoteOpen : TagBlockquoteClose;
		else if (tryParseTag( "cite", si, se)) return open ? TagCiteOpen : TagCiteClose;
		else if (tryParseTag( "poem", si, se)) return open ? TagPoemOpen : TagPoemClose;
		else if (tryParseTag( "div", si, se)) return open ? TagDivOpen : TagDivClose;
		else if (tryParseTag( "span", si, se)) return open ? TagSpanOpen : TagSpanClose;
		else if (tryParseTag( "abbr", si, se)) return open ? TagAbbrOpen : TagAbbrClose;
		else if (tryParseTag( "span", si, se)) return open ? TagSpanOpen : TagSpanClose;
		else if (tryParseTag( "center", si, se)) return open ? TagCenterOpen : TagCenterClose;
		else if (tryParseTag( "small", si, se)) return open ? TagSmallOpen : TagSmallClose;
		else if (tryParseTag( "big", si, se)) return open ? TagBigOpen : TagBigClose;
		else if (tryParseTag( "u", si, se)) return open ? TagUOpen : TagUClose;
		else if (tryParseTag( "s", si, se)) return open ? TagSOpen : TagSClose;
		else if (tryParseTag( "gallery", si, se)) return open ? TagGalleryOpen : TagGalleryClose;
		else if (tryParseTag( "br", si, se)) return open ? TagBr : TagBr;
		else if (tryParseTag( "ol", si, se)) return open ? TagBr : TagBr;
		else if (tryParseTag( "li", si, se)) return open ? TagLiOpen : TagLiClose;
		else if (tryParseTag( "expand", si, se)) return TagComment;
		else if (tryParseTag( "hiero", si, se)) return TagComment;
		else if (tryParseTag( "onlyinclude", si, se)) return TagComment;
		else if (tryParseTag( "noinclude", si, se)) {(void)parseTagContent( "noinclude", si, se); return TagComment;}
		else if (tryParseTag( "score", si, se)) {(void)parseTagContent( "score", si, se); return TagComment;}
		else if (tryParseTag( "timeline", si, se)) {(void)parseTagContent( "timeline", si, se); return TagComment;}
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
	static const char linkchr[] = "$=%@;:/&+()!?.,#-*_\'`~\"^ ";
	char const* ti = m_si;
	while (ti < m_se && isSpace(*ti)) ++ti;
	const char* start = ti;
	while (ti < m_se && (isAlphaNum(*ti) || 0!=std::strchr(linkchr,*ti) || (unsigned char)*ti >= 128)) ++ti;
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
		while (ti < m_se && (!isSpace(*ti) && *ti != '|' && *ti != ']' && *ti != '[' && *ti != '}' && *ti != '{')) ++ti;
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
			for (;si < se && (isAlphaNum(*si)||*si=='#'||*si==':'||*si=='-');++si){}
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
			else
			{
				++m_si;
			}
		}
		if (*m_si == '<')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			if (m_si+1 < m_se && (isAlpha( m_si[1]) || m_si[1] == '/' || m_si[1] == '!'))
			{
				switch (parseTagType( m_si, m_se))
				{
					case UnknwownTagType:
						return WikimediaLexem( WikimediaLexem::Error, 0, std::string("unknown tag ") + outputLineString( m_si-1, m_se, 40));
					case TagNoWikiOpen:
						return WikimediaLexem( WikimediaLexem::NoWiki, 0, parseTagContent( "nowiki", m_si, m_se));
					case TagNoWikiClose:
						start = m_si;
						break;
					case TagCodeOpen:
						return WikimediaLexem( WikimediaLexem::NoWiki, 0, parseTagContent( "code", m_si, m_se));
					case TagCodeClose:
						start = m_si;
						break;
					case TagTtOpen:
						return WikimediaLexem( WikimediaLexem::NoWiki, 0, parseTagContent( "tt", m_si, m_se));
					case TagTtClose:
						start = m_si;
						break;
					case TagSourceOpen:
						return WikimediaLexem( WikimediaLexem::NoWiki, 0, parseTagContent( "source", m_si, m_se));
					case TagSourceClose:
						start = m_si;
						break;
					case TagMathOpen:
						return WikimediaLexem( WikimediaLexem::Math, 0, parseTagContent( "math", m_si, m_se));
					case TagMathClose:
						break;
					case TagChemOpen:
						return WikimediaLexem( WikimediaLexem::Math, 0, parseTagContent( "chem", m_si, m_se));
					case TagChemClose:
						break;
					case TagSupOpen:
						return WikimediaLexem( WikimediaLexem::Math, 0, parseTagContent( "sup", m_si, m_se));
					case TagSupClose:
						break;
					case TagSubOpen:
						return WikimediaLexem( WikimediaLexem::Math, 0, parseTagContent( "sub", m_si, m_se));
					case TagSubClose:
						break;
					case TagGalleryOpen:
						return WikimediaLexem( WikimediaLexem::OpenRef);
					case TagGalleryClose:
						return WikimediaLexem( WikimediaLexem::CloseRef);
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
						return WikimediaLexem( WikimediaLexem::OpenFormat);
					case TagSmallClose:
						return WikimediaLexem( WikimediaLexem::CloseFormat);
					case TagBigOpen:
						return WikimediaLexem( WikimediaLexem::OpenFormat);
					case TagBigClose:
						return WikimediaLexem( WikimediaLexem::CloseFormat);
					case TagUOpen:
						return WikimediaLexem( WikimediaLexem::OpenFormat);
					case TagUClose:
						return WikimediaLexem( WikimediaLexem::CloseFormat);
					case TagLiOpen:
						return WikimediaLexem( WikimediaLexem::ListItem);
					case TagLiClose:
						return WikimediaLexem( WikimediaLexem::EndOfLine);
					case TagSOpen:
						return WikimediaLexem( WikimediaLexem::OpenFormat);
					case TagSClose:
						return WikimediaLexem( WikimediaLexem::CloseFormat);
					case TagBlockquoteOpen:
						return WikimediaLexem( WikimediaLexem::OpenBlockQuote);
					case TagBlockquoteClose:
						return WikimediaLexem( WikimediaLexem::CloseBlockQuote);
					case TagPoemOpen:
						return WikimediaLexem( WikimediaLexem::OpenPoem);
					case TagPoemClose:
						return WikimediaLexem( WikimediaLexem::ClosePoem);
					case TagCiteOpen:
						return WikimediaLexem( WikimediaLexem::OpenRef);
					case TagCiteClose:
						return WikimediaLexem( WikimediaLexem::CloseRef);
					case TagDivOpen:
						return WikimediaLexem( WikimediaLexem::OpenDiv);
					case TagDivClose:
						return WikimediaLexem( WikimediaLexem::CloseDiv);
					case TagComment:
						start = m_si;
						break;
					case TagBr:
						return WikimediaLexem( WikimediaLexem::Text, 0, "\n");
				}
			}
			else
			{
				++m_si;
			}
		}
		else if (m_si[0] == '"')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			char eb = *m_si;
			m_si += 1;
			char const* xi = m_si;
			for (; xi+1 < m_se && *xi != eb && xi[0] != '<' && !(xi[0] == '[' && xi[1] == '[') && !(xi[0] == '{' && xi[1] == '{') && (unsigned char)*xi >= 32 && (xi-m_si) < 100; ++xi){}
			if (xi < m_se && *xi == eb)
			{
				std::string value( m_si, xi-m_si);
				m_si = xi+1;
				return WikimediaLexem( WikimediaLexem::String, 0, value);
			}
			else
			{
				return WikimediaLexem( WikimediaLexem::QuotationMarker);
			}
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
		else if (m_si[0] == '\'' && m_si[1] == '\'' && m_si[2] == ']')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			m_si += 3;
			return WikimediaLexem( WikimediaLexem::OpenDoubleQuote);
		}
		else if (m_si[0] == '[' && m_si[1] == '\'' && m_si[2] == '\'')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			m_si += 3;
			return WikimediaLexem( WikimediaLexem::CloseDoubleQuote);
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
				if (m_si+2 >= m_se) break;

				if (m_si[0] == ']' && m_si[1] == ']')
				{
					m_si += 2;
					break;
				}
				int chlen = strus::utf8charlen( *m_si);
				if (m_si + chlen >= m_se) break;
				if (m_si[chlen+0] == ']' && m_si[chlen+1] == ']')
				{
					const char* chptr = m_si;
					m_si += chlen+2;
					return WikimediaLexem( WikimediaLexem::Char, 0, std::string(chptr,chlen));
				}
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
					while (m_si < m_se && *m_si != ']' && (isAlphaNum(*m_si) || *m_si == '.' || *m_si == ',' || *m_si == '-' || *m_si == '_' || *m_si == '-' || *m_si == '+' || isSpace(*m_si) || (unsigned char)*m_si > 128))
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
				char const* xi = m_si;
				char eb = ']';
				for (; xi+1 < m_se && *xi != eb && xi[0] != '<' && !(xi[0] == '[' && xi[1] == '[') && !(xi[0] == '{' && xi[1] == '{') && (unsigned char)*xi >= 32 && (xi-m_si) < 40; ++xi){}
				if (xi < m_se && *xi == eb)
				{
					std::string value( m_si, xi-m_si);
					m_si = ++xi;
					return WikimediaLexem( WikimediaLexem::Char, 0, value);
				}
				else
				{
					return WikimediaLexem( WikimediaLexem::Text, 0, " [");
				}
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
				m_si = skipStyle( m_si, m_se);
				while (m_si < m_se && *m_si == 32) ++m_si;
				if (m_si+2 < m_se && m_si[0] == '|' && m_si[1] != '|') ++m_si;
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
					m_si = skipStyle( m_si, m_se);
					std::string name = tryParseIdentifier( '=');
					return WikimediaLexem( WikimediaLexem::TableColDelim);
				}
			}
			else if (*m_si == '*')
			{
				int lidx = countAndSkip( m_si, m_se, '*', 5);
				if (lidx >= 1)
				{
					return WikimediaLexem( WikimediaLexem::ListItem, lidx, "");
				}
			}
			else if (m_si+2 < m_se && m_si[0] == ':' && m_si[1] == ';')
			{
				m_si += 2;
				return WikimediaLexem( WikimediaLexem::HeadingItem);
			}
			else if (m_si+2 < m_se && m_si[0] == ':' && m_si[1] == '#')
			{
				m_si += 2;
				return WikimediaLexem( WikimediaLexem::ListItem, 1, "");
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


