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
#include <cstdarg>

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

static bool isIdentifierChar( char ch, bool withDash)
{
	return isAlphaNum(ch) || ch == '_' || (withDash && ch == '-');
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

static char const* skipToEoln( char const* si, char const* se)
{
	for (; si < se && *si != '\n'; ++si){}
	return si;
}

static char const* skipString( char const* si, char const* se)
{
	char eb = *si;
	++si;
	for (; si < se && *si != '\n' && *si != eb; ++si){}
	if (si < se && *si == eb)
	{
		return si+1;
	}
	return NULL;
}

static bool parseString( std::string& res, char const*& si, char const* se, bool tolerant)
{
	char const* start = si+1;
	char const* end = skipString( si, se);
	if (end)
	{
		res = std::string( start, end-start-1);
		si = end;
		return true;
	}
	else if (tolerant)
	{
		end = skipToEoln( si, se);
		if (end)
		{
			res = std::string( start, end-start);
			si = end;
			return true;
		}
	}
	return false;
}

bool isTokenDelimiter( char ch)
{
	return ch == '|' || ch == '>' || ch == '<' || ch == ']'|| ch == '[' || ch == '}' || ch == '{';
}

static char const* skipToken( char const* si, char const* se)
{
	int sidx=0;
	while (si < se && !isSpace(*si) && !isTokenDelimiter(*si)) {++si,++sidx;}
	if (si < se && sidx)
	{
		return si;
	}
	return NULL;
}

static bool parseToken( std::string& res, char const*& si, char const* se)
{
	char const* start = si;
	char const* end = skipToken( si, se);
	if (end)
	{
		res = std::string( start, end-start);
		si = end;
		return true;
	}
	return false;
}

static char const* skipSpaces( char const* si, char const* se)
{
	for (;si < se && (*si == ' '||*si == '\t');++si){}
	return si;
}

static char const* skipSpacesAndComments( char const* si, char const* se)
{
	while (si < se)
	{
		si = skipSpaces( si, se);
		if (si < se && si[0] == '<' && si[1] == '!' && si[2] == '-' && si[3] == '-')
		{
			const char* end = findPattern( si+4, se, "-->");
			if (end) si = end; else return si;
		}
		else
		{
			return si;
		}
	}
	return si;
}

static char const* skipIdentifier( char const* si, char const* se, bool withDash)
{
	int sidx=0;
	for (;si < se && isIdentifierChar(*si, withDash);++si,++sidx){}
	return sidx ? si : NULL;
}

static bool parseIdentifier( std::string& res, char const*& si, char const* se, bool withDash)
{
	char const* start = si;
	char const* end = skipIdentifier( si, se, withDash);
	if (end)
	{
		res = std::string( start, end-start);
		si = end;
		return true;
	}
	return false;
}

static std::string parseTagContent( const char* tagname, char const*& si, const char* se)
{
	const char* end = (const char*)std::memchr( si, '<', se-si);
	for (; end-si < 256 && end+1 < se; end = (const char*)std::memchr( end+1, '<', se-end-1))
	{
		const char* tg = end+1;
		if (*tg != '/') continue;
		++tg;
		while (*tg && isSpace(*tg)) ++tg;
		const char* name = tg;
		while (tg < se && isAlpha(*tg)) ++tg;
		if (isEqual( tagname, name, tg-name))
		{
			while (*tg && isSpace(*tg)) ++tg;
			if (tg < se && *tg == '>') ++tg;
			std::string rt( si, end-si);
			si = tg;
			return rt;
		}
	}
	return std::string();
}

enum TagType {
	UnknwownTagType,
	TagNoWikiOpen,
	TagNoWikiClose,
	TagVarOpen,
	TagVarClose,
	TagCodeOpen,
	TagCodeClose,
	TagTimestampOpen,
	TagTimestampClose,
	TagTtOpen,
	TagTtClose,
	TagSourceOpen,
	TagSourceClose,
	TagSyntaxHighlightOpen,
	TagSyntaxHighlightClose,
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
	TagPreOpen,
	TagPreClose,
	TagInsOpen,
	TagInsClose,
	TagImageMapOpen,
	TagImageMapClose,
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
	TagIOpen,
	TagIClose,
	TagPOpen,
	TagPClose,
	TagLiOpen,
	TagLiClose,
	TagTrOpen,
	TagTrClose,
	TagHrOpen,
	TagHrClose,
	TagTdOpen,
	TagTdClose,
	TagSOpen,
	TagSClose,
	TagQOpen,
	TagQClose,
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

static bool findEndTag( char const*& si, const char* se, int maxlen)
{
	int ti = 0;
	const char* start = si;

	for (;si < se && *si != '<' && *si != '>' && ti < maxlen; ++si,++ti){}
	if (si < se && *si == '>')
	{
		++si;
		return true;
	}
	si = start;
	return false;
}

static bool tryParseTag( const char* tagnam, char const*& si, const char* se)
{
	const char* start = si;
	int ti = 0;
	for (;si < se && tagnam[ti] && tagnam[ti] == (*si|32); ++si,++ti){}
	if (!tagnam[ti] && (*si == ' ' || *si == '/' || *si == '>') && findEndTag( si, se, 256))
	{
		return true;
	}
	si = start;
	return false;
}

static bool tryParseAnyTag( char const*& si, const char* se)
{
	const char* start = si;
	int ti = 0;
	for (;si < se && isAlpha(*si) && ti < 40; ++si,++ti){}
	if (si < se && ti < 40 && findEndTag( si, se, 40))
	{
		return true;
	}
	si = start;
	return false;
}

static bool tryParseTagDefStart( char const*& si, const char* se)
{
	int attrcnt = 0;
	const char* start = si;
	si = skipIdentifier( si, se, true/*with dash*/);

	if (si && si<se && *si == ' ') for (;;)
	{
		si = skipSpaces( si, se);
		if (!isAlpha(*si))
		{
			si=start;
			return attrcnt>0;
		}
		si = skipIdentifier( si, se, true/*with dash*/);
		si = skipSpaces( si, se);
	
		if (si < se && (*si == '=' || *si == ':'))
		{
			si = skipSpaces( si+1, se);
			if (si < se && (*si == '"' || *si == '\''))
			{
				si = skipString( si, se);
				if (!si)
				{
					si = start;
					return attrcnt>0;
				}
				++attrcnt;
				start = si;
			}
			else if (si < se && (isIdentifierChar(*si, true/*with dash*/) || *si == '#'))
			{
				si = skipToken( si, se);
				if (!si)
				{
					si = start;
					return attrcnt>0;
				};
				++attrcnt;
				start = si;
			}
			else
			{
				si = start;
				return attrcnt>0;
			}
		}
		else
		{
			si = start;
			return attrcnt>0;
		}
	}
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
		char const* sn = si;
		if (findEndTag( sn, se, 256))
		{
			if (*(sn-2) == '/')
			{
				si = sn;
				return TagComment;
				//.... immediate close tags not encolsing any content are considered unimportant for text retrieval and thus marked as comments.
			}
		}
		if (tryParseTag( "nowiki", si, se)) return open ? TagNoWikiOpen : TagNoWikiClose;
		else if (tryParseTag( "timestamp", si, se)) return open ? TagTimestampOpen : TagTimestampClose;
		else if (tryParseTag( "code", si, se)) return open ? TagCodeOpen : TagCodeClose;
		else if (tryParseTag( "var", si, se)) return open ? TagVarOpen : TagVarClose;
		else if (tryParseTag( "tt", si, se)) return open ? TagTtOpen : TagTtClose;
		else if (tryParseTag( "syntaxhighlight", si, se)) return open ? TagSyntaxHighlightOpen : TagSyntaxHighlightClose;
		else if (tryParseTag( "source", si, se)) return open ? TagSourceOpen : TagSourceClose;
		else if (tryParseTag( "math", si, se)) return open ? TagMathOpen : TagMathClose;
		else if (tryParseTag( "chem", si, se)) return open ? TagChemOpen : TagChemClose;
		else if (tryParseTag( "sup", si, se)) return open ? TagSupOpen : TagSupClose;
		else if (tryParseTag( "sub", si, se)) return open ? TagSubOpen : TagSubClose;
		else if (tryParseTag( "pre", si, se)) return open ? TagPreOpen : TagPreClose;
		else if (tryParseTag( "ins", si, se)) return open ? TagInsOpen : TagInsClose;
		else if (tryParseTag( "imagemap", si, se)) return open ? TagImageMapOpen : TagImageMapClose;
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
		else if (tryParseTag( "q", si, se)) return open ? TagQOpen : TagQClose;
		else if (tryParseTag( "i", si, se)) return open ? TagIOpen : TagIClose;
		else if (tryParseTag( "p", si, se)) return open ? TagPOpen : TagPClose;
		else if (tryParseTag( "gallery", si, se)) return open ? TagGalleryOpen : TagGalleryClose;
		else if (tryParseTag( "br", si, se)) return open ? TagBr : TagBr;
		else if (tryParseTag( "ol", si, se)) return open ? TagBr : TagBr;
		else if (tryParseTag( "ul", si, se)) return open ? TagBr : TagBr;
		else if (tryParseTag( "li", si, se)) return open ? TagLiOpen : TagLiClose;
		else if (tryParseTag( "tr", si, se)) return open ? TagTrOpen : TagTrClose;
		else if (tryParseTag( "hr", si, se)) return open ? TagHrOpen : TagHrClose;
		else if (tryParseTag( "td", si, se)) return open ? TagTdOpen : TagTdClose;
		else if (open && tryParseTag( "noinclude", si, se)) {(void)parseTagContent( "noinclude", si, se); return TagComment;}
		else if (open && tryParseTag( "score", si, se)) {(void)parseTagContent( "score", si, se); return TagComment;}
		else if (open && tryParseTag( "timeline", si, se)) {(void)parseTagContent( "timeline", si, se); return TagComment;}
		else if (tryParseTag( "please", si, se)) return open ? TagComment : TagComment;
		else if (tryParseAnyTag( si, se)) return TagBr;
		else if (tryParseTagDefStart( si, se)) return TagBr;
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
		while (ti < m_se && (isIdentifierChar(*ti,true/*with dash*/) || isSpace(*ti)))
		{
			char ch = *ti++;
			if (isAlpha(ch))
			{
				rt.push_back( ch|32);
			}
			else if (isSpace(ch) && rt.size()>0 && rt[ rt.size()-1] != ' ')
			{
				rt.push_back( ' ');
			}
			else
			{
				rt.push_back( ch);
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

std::string WikimediaLexer::tryParseURLPath()
{
	char const* ti = m_si;
	while (ti < m_se && !isSpace(*ti) && !isTokenDelimiter(*ti)) ++ti;
	std::string linkid( m_si, ti-m_si);
	m_si = ti;
	if (m_si < m_se && isSpace(*m_si))
	{
		while (m_si < m_se && isSpace(*m_si)) ++m_si;
		if (m_si < m_se && *m_si != '|' && *m_si != ']') --m_si;
	}
	return linkid;
}

static bool isUrlCandidate( char const* si, const char* se)
{
	int pcnt = 0;
	for (; pcnt < 6 && si < se && isAlpha(*si); ++si, ++pcnt){}
	return (si+3 < se && si[0] == ':' && si[1] == '/' && si[2] == '/');
}

std::string WikimediaLexer::tryParseURL()
{
	char const* start = m_si;
	char const* ti = m_si;
	while (ti < m_se && isSpace(*ti)) ++ti;
	while (ti < m_se && isAlpha(*ti)) ++ti;
	if (ti+3 < m_se && ti[0] == ':' && ti[1] == '/' && ti[2] == '/')
	{
		char const* pathstart = m_si;
		m_si = ti + 3;
		char const* pathend = m_si;
		std::string path = tryParseURLPath();
		if (!path.empty())
		{
			return std::string( pathstart, pathend-pathstart) + path;
		}
	}
	m_si = start;
	return std::string();
}

std::string WikimediaLexer::tryParseFilePath()
{
	std::string rt = tryParseURLPath();
	if (!rt.empty())
	{
		char const* si = rt.c_str() + rt.size() -1;
		int sidx = 0;
		for (; si >= rt.c_str() && sidx <= 6 && isAlpha(*si); --si,++sidx){}
		if (si >= rt.c_str() && *si == '.') return rt;
	}
	return std::string();
}

static int hexNumCount( char const* si, const char* se)
{
	int rt = 0;
	while (si < se && ((*si >= '0' && *si <= '9') || (((*si|32) >= 'a') && ((*si|32) <= 'f')))) {++si;++rt;}
	return rt;
}

static int decNumCount( char const* si, const char* se)
{
	int rt = 0;
	while (si < se && *si >= '0' && *si <= '9') {++si;++rt;}
	return rt;
}

static bool isBibRefCandidate( char const* si, const char* se)
{
	int mb_cnt = 0;
	int hc_sum = 0;
	int hc = hexNumCount( si, se);
	while (hc >= 1 && si < se)
	{
		si += hc;
		if (si[hc] == '-') si += 1;
		hc = hexNumCount( si, se);
		hc_sum += hc;
		mb_cnt += 1;
	}
	return mb_cnt >= 2 && hc_sum >= 5;
}

static bool isBookRefCandidate( char const* si, const char* se)
{
	int hc = decNumCount( si, se);
	if (hc >= 2 && si < se && si[hc] == '.')
	{
		si += hc+1;
		hc = decNumCount( si, se);
		return (hc >= 4 && si < se && si[hc] == '/' && isAlphaNum( si[hc+1]));
	}
	return false;
}

struct BibRefPart
{
	const char* start;
	int size;
};

static bool parseBibRefPart( BibRefPart& res, char const*& si, const char* se)
{
	res.start = si;
	res.size = hexNumCount( si, se-1);
	if (res.size >= 1)
	{
		si += res.size;
		return true;
	}
	return false;
}

struct BibRef
{
	enum {MaxNofParts=16};
	BibRefPart ar[ MaxNofParts];
	int size;

	std::string tostring() const
	{
		std::string rt;
		int ai = 0;
		for (; ai < size; ++ai)
		{
			if (ai) rt.push_back('-');
			rt.append( ar[ ai].start, ar[ ai].size);
		}
		return rt;
	}
};

static bool parseBibRef( BibRef& res, char const*& si, const char* se)
{
	int chcnt = 0;
	res.size = 0;
	while (res.size < BibRef::MaxNofParts && parseBibRefPart( res.ar[ res.size], si, se))
	{
		chcnt += res.ar[ res.size].size;
		++res.size;
		if (si+1 < se && *si != '-') break;
		++si;
	}
	if (res.size == 3
		&& res.ar[0].size == 4
		&& res.ar[1].size == 2
		&& res.ar[2].size == 2
		&& (res.ar[0].start[0] == '1' || res.ar[0].start[0] == '2'))
	{
		// ...date
		return false;
	}
	else if (res.size == 2
		&& res.ar[0].size == 4
		&& res.ar[1].size == 4
		&& (res.ar[0].start[0] == '1' || res.ar[0].start[0] == '2')
		&& (res.ar[1].start[0] == '1' || res.ar[1].start[0] == '2'))
	{
		// ...year range
		return false;
	}
	else
	{
		return (si < se && *si != '-' && res.size >= 2 && chcnt >= 8);
	}
}

std::string WikimediaLexer::tryParseBibRef()
{
	BibRef res;
	char const* si = m_si;
	if (parseBibRef( res, si, m_se))
	{
		m_si = si;
		return res.tostring();
	}
	else
	{
		return std::string();
	}
}

std::string WikimediaLexer::tryParseBookRef()
{
	char const* start = m_si;
	int dc;
	dc = decNumCount( m_si, m_se-1);
	if (2 <= dc && m_si[dc] == '.')
	{
		m_si += dc+1;
	}
	else
	{
		m_si = start;
		return std::string();
	}
	dc = decNumCount( m_si, m_se-1);
	if (4 <= dc && m_si[dc] == '/')
	{
		m_si += dc+1;
	}
	else
	{
		m_si = start;
		return std::string();
	}
	int ac = 0;
	int mc = 0;
	while (m_si < m_se && (isAlphaNum( *m_si) || *m_si == '.' || *m_si == '_' || *m_si == '(' || *m_si == ')' || *m_si == '-'))
	{
		if (*m_si == '.' || *m_si == '(' || *m_si == ')' || *m_si == '-' || isDigit(*m_si)) ++mc;
		++ac;
		++m_si;
	}
	if (ac > 4 && mc > 0 && m_si < m_se && (isSpace(*m_si) || isTokenDelimiter(*m_si) || *m_si == '\'' || *m_si == '\"'))
	{
		return std::string( start, m_si-start);
	}
	else
	{
		m_si = start;
		return std::string();
	}
}

static int parseKeyWord( const char*& si, const char* se, int nn, ...)
{
	int ii = 0;
	va_list kl;
	va_start( kl, nn);
	for ( ii=0; ii<nn; ii++)
	{
		char const* ki = va_arg( kl, const char*);
		char const* vi = si;
		for (; vi<se && *vi == *ki; ++ki,++vi){}
		if (!*ki && (vi == se || !isAlphaNum(*vi)))
		{
			si = vi;
			return ii;
		}
	}
	va_end( kl);
	return -1;
	
}

static bool isIsbnRefCandidate(  char const* si, const char* se)
{
	return (si+4 < se && ((si[0]|32) == 'i' || (si[0]|32) == 'a') && 0<=parseKeyWord( si, se, 2, "ISBN", "ASIN"));
}

std::string WikimediaLexer::tryParseIsbnRef()
{
	std::string rt;
	int kw;
	const char* start = m_si;
	switch (kw=parseKeyWord( m_si, m_se, 2, "ISBN", "ASIN"))
	{
		case -1: break;
		case 0:
		case 1:
		{
			m_si = skipSpaces( m_si, m_se);
			if (m_si && m_si < m_se && *m_si == ':') {++m_si;m_si = skipSpaces( m_si, m_se);}

			char const* end = skipIdentifier( m_si, m_se, true/*with dash*/);
			if (end - m_si >= 10)
			{
				const char* idstart = m_si;
				m_si = end;
				return std::string((kw==0)?"ISBN":"ASIN") + " " + std::string( idstart, end-idstart);
			}
		}
	}
	m_si = start;
	return std::string();
}

static bool isRepPatternCandidate( char const* si, const char* se)
{
	int df = 0;
	int minlen = 16;
	if ((unsigned char)*si < 128 && si + minlen < se)
	{
		if (si[0] == si[1]) df = 1;
		else if (si[0] == si[2]) df = 2;
		else if (si[0] == si[3]) df = 3;
		if (!df) return false;
		for (int di=0; di<df; ++di) if (isSpace(si[di]) || isDigit(si[di] || (unsigned char)*si >= 128)) return false;

		int ii=0;
		for (; ii<minlen && si[ii] == si[ii+df]; ++ii){}
		return ii >= minlen;
	}
	return false;
}

std::string WikimediaLexer::tryParseRepPattern()
{
	std::string rt;

	int df = 0;
	int minlen = 16;
	if ((unsigned char)*m_si < 128 && m_si + minlen < m_se)
	{
		if (m_si[0] == m_si[1]) df = 1;
		else if (m_si[0] == m_si[2]) df = 2;
		else if (m_si[0] == m_si[3]) df = 3;
		if (!df) return std::string();
		for (int di=0; di<df; ++di) if (isSpace(m_si[di]) || isDigit(m_si[di] || (unsigned char)*m_si >= 128)) return std::string();

		int ii=0;
		for (; m_si+ii+df<m_se && m_si[ii] == m_si[ii+df]; ++ii){}
		if (ii >= minlen)
		{
			rt.append( m_si, ii+df);
			m_si += ii+df;
		}
	}
	return rt;
}

static bool isBigHexNumCandidate(  char const* si, const char* se)
{
	if (si+8 < se && si[0] == '0' && si[1] == 'x' && 6<hexNumCount( si+2,se)) return true;
	if (si+7 < se && si[0] == '#' && 6<hexNumCount( si+1,se)) return true;
	if (si+12 < se && 6<hexNumCount( si+1,se)) return true;
	return false;
}

std::string WikimediaLexer::tryParseBigHexNum()
{
	std::string rt;
	
	if (m_si+10 < m_se && m_si[0] == '0' && m_si[1] == 'x')
	{
		int hc = hexNumCount( m_si+2,m_se);
		if (hc >= 6 && (m_si == m_se || !isAlpha( m_si[2+hc])))
		{
			const char* start = m_si+2;
			m_si = m_si+2+hc;
			return std::string( start, m_si-start);
		}
	}
	else if (m_si+7 < m_se && m_si[0] == '#')
	{
		int hc = hexNumCount( m_si+1,m_se);
		if (hc >= 6 && (m_si == m_se || !isAlpha( m_si[1+hc])))
		{
			const char* start = m_si+1;
			m_si = m_si+1+hc;
			return std::string( start, m_si-start);
		}
	}
	else
	{
		int hc = hexNumCount( m_si,m_se);
		if (hc >= 12 && (m_si == m_se || !isAlpha( m_si[hc])))
		{
			const char* start = m_si;
			m_si = m_si+hc;
			return std::string( start, m_si-start);
		}
	}
	return std::string();
}

static bool isTimestampCandidate( char const* si, const char* se)
{
	int dlen = decNumCount( si, se);
	return (si+13 < se && dlen >= 8 && si[dlen] == 'T' && decNumCount( si+dlen+1, se) >= 4);
}

std::string WikimediaLexer::tryParseTimestamp()
{
	std::string rt;
	int dlen = decNumCount( m_si, m_se);
	if (m_si+13 < m_se && dlen >= 8 && m_si[dlen] == 'T')
	{
		int glen = decNumCount( m_si+dlen+1, m_se);
		if (glen >= 4 && m_si[dlen+1+glen] == 'Z' && !isAlphaNum(m_si[dlen+1+glen+1]))
		{
			rt.append( m_si, dlen+1+glen+1);
			m_si += dlen+1+glen+1;
		}
	}
	return rt;
}

static int charClassChangeCount( char const* si, const char* se)
{
	int rt = 0;
	int cl = 0;
	int max_cl = 0;
	for (int sidx=0; si < se; ++si,++sidx)
	{
		int prev_cl = cl;
		if (*si >= 'A' && *si <= 'Z') cl = 1;
		else if (*si >= 'a' && *si <= 'z') cl = 2;
		else if (*si == '_') cl = 3;
		else if (*si >= '0' && *si <= '9') cl = 4;
		else if ((unsigned char)*si >= 128) cl = 5;
		else return -1;

		if (prev_cl && prev_cl != cl)
		{
			if (sidx == 1 && prev_cl == 1 && cl == 2) continue;
			if (max_cl <= 2 && cl == 4) continue;
			if (prev_cl <= 2 && cl == 5) continue;
			if (prev_cl == 5 && cl <= 2) continue;
			++rt;
		}
		if (cl > max_cl) max_cl = cl;
	}
	return rt;
}

static bool isCodeCandidate( char const* si, const char* se)
{
	return (se - si > 12 && 4 <= charClassChangeCount( si, si+12));
}

std::string WikimediaLexer::tryParseCode()
{
	std::string rt;
	const char* tk = skipIdentifier( m_si, m_se, false/*with dash*/);
	if (tk - m_si > 12 && 4 <= charClassChangeCount( m_si, tk))
	{
		rt.append( m_si, tk-m_si);
		m_si = tk;
		return rt;
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

static void parseAttributes( char const*& si, char const* se, char endMarker, char altEndMarker, std::map<std::string,std::string>& attributes)
{
	const char* start = si;
	si = skipSpaces( si, se);
	if (si == se || *si == endMarker || *si == altEndMarker) return;

	for (;;)
	{
		std::string name;
		std::string value;

		if (!parseIdentifier( name, si, se, true/*with dash*/)) goto REWIND;
		si = skipSpaces( si, se);

		if (si < se && (*si == '=' || *si == ':'))
		{
			++si;
			si = skipSpaces( si, se);
			if (si < se && (*si == '"' || *si == '\''))
			{
				while (si < se && (*si == '"' || *si == '\''))
				{
					if (!value.empty()) value.push_back(' ');
					if (!parseString( value, si, se, true/*tolerant*/)) goto REWIND;
					si = skipSpaces( si, se);
				}
			}
			else if (si < se && (*si == '#' || isIdentifierChar(*si, true/*with dash*/)))
			{
				if (!parseToken( value, si, se)) goto REWIND;
			}
			else if (si < se && (*si == endMarker || *si == altEndMarker))
			{
				return;
			}
			else
			{
				goto REWIND;
			}
			attributes[ name] = value;

			si = skipSpacesAndComments( si, se);
			if (si == se || *si == endMarker || *si == altEndMarker) return;
		}
		else
		{
			goto REWIND;
		}
	}
	return;

REWIND:
	attributes.clear();
	si = start;
}

static const char* skipUrlParameters( char const* si, char const* se)
{
	char const* rt = 0;
	while (si < se && *si == '&')
	{
		si++;
		si = skipIdentifier( si, se, true/*with dash*/);
		if (!si) break;

		if (si < se && *si == '=')
		{
			++si;
			if (si < se && (*si == '"' || *si == '\''))
			{
				si = skipString( si, se);
				if (!si) break;
			}
			else
			{
				int sidx=0;
				while (si < se && !isSpace(*si) && *si != '&' && !isTokenDelimiter(*si)) {++si,++sidx;}
			}
		}
		else
		{
			break;
		}
		rt = si;
	}
	return rt;
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
		if ((unsigned char)*m_si >= 128)
		{
			int chlen = strus::utf8charlen( *m_si);
			m_si += chlen;
		}
		else if (*m_si == '=' && m_curHeading)
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
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
		else if (*m_si == '<')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			if (m_si+8 < m_se && (0==std::memcmp( m_si, "<http://", 8) || 0==std::memcmp( m_si, "<https://", 9)))
			{
				char const* start = m_si;
				++m_si;
				for (; m_si < m_se && *m_si != '>'; ++m_si){}
				if (m_si < m_se)
				{
					std::string url( start+1, m_si-(start+1));
					++m_si;
					return WikimediaLexem( WikimediaLexem::Url, 0, url);
				}
				else
				{
					return WikimediaLexem( WikimediaLexem::Error, 0, std::string("unknown tag ") + outputLineString( m_si-1, m_se, 40));
				}
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
					case TagTimestampOpen:
						return WikimediaLexem( WikimediaLexem::Timestamp, 0, parseTagContent( "timestamp", m_si, m_se));
					case TagTimestampClose:
						start = m_si;
						break;
					case TagCodeOpen:
						return WikimediaLexem( WikimediaLexem::NoWiki, 0, parseTagContent( "code", m_si, m_se));
					case TagCodeClose:
						start = m_si;
						break;
					case TagVarOpen:
						return WikimediaLexem( WikimediaLexem::NoWiki, 0, parseTagContent( "var", m_si, m_se));
					case TagVarClose:
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
					case TagSyntaxHighlightOpen:
						return WikimediaLexem( WikimediaLexem::NoWiki, 0, parseTagContent( "syntaxhighlight", m_si, m_se));
					case TagSyntaxHighlightClose:
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
					case TagImageMapOpen:
						return WikimediaLexem( WikimediaLexem::OpenRef);
					case TagImageMapClose:
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
					case TagTrOpen:
						return WikimediaLexem( WikimediaLexem::TableRowDelim);
					case TagTrClose:
						return WikimediaLexem( WikimediaLexem::EndOfLine);
					case TagHrOpen:
						return WikimediaLexem( WikimediaLexem::TableHeadDelim);
					case TagHrClose:
						return WikimediaLexem( WikimediaLexem::EndOfLine);
					case TagTdOpen:
						return WikimediaLexem( WikimediaLexem::TableColDelim);
					case TagTdClose:
						return WikimediaLexem( WikimediaLexem::EndOfLine);
					case TagSOpen:
						return WikimediaLexem( WikimediaLexem::OpenFormat);
					case TagSClose:
						return WikimediaLexem( WikimediaLexem::CloseFormat);
					case TagQOpen:
						return WikimediaLexem( WikimediaLexem::OpenFormat);
					case TagQClose:
						return WikimediaLexem( WikimediaLexem::CloseFormat);
					case TagPreOpen:
						return WikimediaLexem( WikimediaLexem::OpenFormat);
					case TagPreClose:
						return WikimediaLexem( WikimediaLexem::CloseFormat);
					case TagInsOpen:
						return WikimediaLexem( WikimediaLexem::OpenFormat);
					case TagInsClose:
						return WikimediaLexem( WikimediaLexem::CloseFormat);
					case TagIOpen:
						return WikimediaLexem( WikimediaLexem::OpenFormat);
					case TagIClose:
						return WikimediaLexem( WikimediaLexem::CloseFormat);
					case TagPOpen:
						return WikimediaLexem( WikimediaLexem::OpenFormat);
					case TagPClose:
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
			for (; xi+1 < m_se && *xi != eb && xi[0] != '<' && !(xi[0] == ':' && xi[1] == '/' && xi[2] == '/') && !(xi[0] == '\'' && xi[1] == '\'') && !(xi[0] == '[' && xi[1] == '[') && !(xi[0] == '{' && xi[1] == '{') && (unsigned char)*xi >= 32 && (xi-m_si) < 128; ++xi)
			{}
			if (xi < m_se && *xi == eb)
			{
				std::string value( m_si, xi-m_si);
				m_si = xi+1;
				if (charClassChangeCount( value.c_str(), value.c_str() + value.size()) >= 4)
				{
					return WikimediaLexem( WikimediaLexem::Code, 0, value);
				}
				else
				{
					return WikimediaLexem( WikimediaLexem::String, 0, value);
				}
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
		else if (m_si[0] == '\'' && m_si[1] == '\'')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			int sidx = 0;
			if (m_si[2] == ']')
			{
				char const* ci = m_si+2;
				int cnt = 20;
				while (ci < m_se && --cnt>=0 && (unsigned char)*ci > 32 && !isTokenDelimiter(*ci) && *ci != '\"') ++ci;
				if (ci < m_se && ci[0] == ']' && ci[1] == '\'' && ci[2] == '\'')
				{
					const char* tkstart = m_si+2;
					int tksize = ci - tkstart;
					m_si = ci + 2;
					return WikimediaLexem( WikimediaLexem::Char, 0, std::string( tkstart, tksize));
				}
			}
			for (; m_si < m_se && *m_si == '\''; ++sidx,++m_si){}
			if (sidx >= 6)
			{
				return WikimediaLexem( WikimediaLexem::NoData, 0, std::string(start,sidx));
			}
			return WikimediaLexem( WikimediaLexem::MultiQuoteMarker, sidx, "");
		}
		else if (*m_si == '[')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			++m_si;
			if (m_si == m_se) break;

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
				std::string linkid;
				if (isUrlCandidate( m_si, m_se))
				{
					linkid = tryParseURL();
					return WikimediaLexem( WikimediaLexem::OpenWWWLink, 0, linkid);
				}
				else
				{
					linkid = tryParseLinkId();
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
			}
			else if (isAlphaNum(*m_si) || isSpace(*m_si) || *m_si == '/' || *m_si == '.' || *m_si == '-' || *m_si == '+')
			{
				m_si = skipSpaces( m_si, m_se);
				std::string linkid;

				if (m_si+2 < m_se && m_si[0] == '/' && m_si[1] == '/')
				{
					std::string urlpath = tryParseURLPath();
					if (!urlpath.empty()) linkid = std::string("http:") + urlpath;
				}
				else if (isUrlCandidate( m_si, m_se))
				{
					linkid = tryParseURL();
					if (linkid.empty())
					{
						std::string filepath = tryParseFilePath();
						if (!filepath.empty()) linkid = std::string("file://") + filepath;
					}
				}
				if (linkid.empty())
				{
					while (m_si < m_se && *m_si != ']' && (isAlphaNum(*m_si) || *m_si == '.' || *m_si == ',' || *m_si == '_' || *m_si == '-' || *m_si == '+' || isSpace(*m_si) || (unsigned char)*m_si > 128))
					{
						++m_si;
					}
					if (m_si < m_se && *m_si == ']') ++m_si;
				}
				else if (m_si < m_se && (isTokenDelimiter(*m_si) || isSpace(*m_si)))
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
			if (m_si == m_se) break;

			if (*m_si == '{')
			{
				++m_si;
				const char* start = m_si;
				
				for (; m_si != m_se && !isTokenDelimiter( *m_si); ++m_si){}
				if (m_si +1 < m_se && m_si[0] == '}' && m_si[1] == '}')
				{
					std::string title( strus::string_conv::trim( std::string( start, m_si - start)));
					m_si += 2;
					return WikimediaLexem( WikimediaLexem::Markup, 0, title);
				}
				m_si = start;
				while (m_si < m_se && *m_si != '}' && *m_si != '|' && (isAlphaNum(*m_si) || *m_si == '_' || *m_si == '-' || isSpace(*m_si)))
				{
					++m_si;
				}
				if (m_si < m_se && (*m_si == '}' || *m_si == '|'))
				{
					std::string citid( strus::string_conv::trim( std::string( start, m_si - start)));
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
				while (m_si < m_se && *m_si == '-') ++m_si;
				m_si = skipSpaces( m_si, m_se);
				if (*m_si == '!') ++m_si;

				std::map<std::string,std::string> attributes;
				std::map<std::string,std::string> aa;
				bool more;
				do
				{
					more = false;
					parseAttributes( m_si, m_se,  '|', '\n', aa);
					if (m_si < m_se && *m_si == '|') {more=true; ++m_si;}
					attributes.insert( aa.begin(), aa.end());
				} while (more);
				return WikimediaLexem( WikimediaLexem::OpenTable, 0, "", attributes);
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
				std::map<std::string,std::string> attributes;
				parseAttributes( m_si, m_se,  '|', '\n', attributes);
				if (m_si+2 < m_se && m_si[0] == '|' && m_si[1] != '|') ++m_si;
				return WikimediaLexem( WikimediaLexem::DoubleColDelim, 0, "", attributes);
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
			std::map<std::string,std::string> attributes;
			parseAttributes( m_si, m_se,  '|', '\n', attributes);
			if (m_si < m_se && *m_si == '|') ++m_si;
			return WikimediaLexem( WikimediaLexem::TableHeadDelim, 0, "", attributes);
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
				std::map<std::string,std::string> attributes;
				parseAttributes( m_si, m_se, '|', '\n', attributes);
				if (m_si < m_se && *m_si == '|') ++m_si;
				return WikimediaLexem( WikimediaLexem::TableHeadDelim, 0, "", attributes);
			}
			else if (*m_si == '|')
			{
				m_si = skipSpaces( m_si+1, m_se);
				if (m_si == m_se) break;

				if (*m_si == '-')
				{
					while (m_si < m_se && *m_si == '-') ++m_si;
					m_si = skipSpaces( m_si, m_se);
					if (*m_si == '!') ++m_si;
					std::map<std::string,std::string> attributes;
					parseAttributes( m_si, m_se,  '|', '\n', attributes);
					if (m_si < m_se && *m_si == '|') ++m_si;
					return WikimediaLexem( WikimediaLexem::TableRowDelim, 0, "", attributes);
				}
				else if (*m_si == '+')
				{
					while (*m_si == '+') ++m_si;
					std::map<std::string,std::string> attributes;
					parseAttributes( m_si, m_se,  '|', '\n', attributes);
					if (m_si < m_se && *m_si == '|') ++m_si;
					return WikimediaLexem( WikimediaLexem::TableTitle, 0, "", attributes);
				}
				else if (*m_si == '!')
				{
					++m_si;
					std::map<std::string,std::string> attributes;
					parseAttributes( m_si, m_se, '|', '\n', attributes);
					if (m_si < m_se && *m_si == '|') ++m_si;
					return WikimediaLexem( WikimediaLexem::TableHeadDelim, 0, "", attributes);
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
					std::map<std::string,std::string> attributes;
					parseAttributes( m_si, m_se, '|', '\n', attributes);
					if (m_si < m_se && *m_si == '|') ++m_si;
					std::string name = tryParseIdentifier( '=');
					return WikimediaLexem( WikimediaLexem::TableColDelim, 0, name, attributes);
				}
			}
			else if (*m_si == '*')
			{
				int lidx = countAndSkip( m_si, m_se, '*', 18);
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
		else if (m_si + 3 < m_se && m_si[0] == ':' && m_si[1] == '/' && m_si[2] == '/')
		{
			const char* si_bk = m_si;
			char const* ti = m_si;
			while (m_si - ti < 7 &&  ti > start && isAlpha(*(ti-1))) --ti;
			if (m_si - ti >= 3 && m_si - ti < 7 && ti >= start)
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
		else if (isTimestampCandidate( m_si, m_se))
		{
			std::string timestmp( tryParseTimestamp());
			if (!timestmp.empty())
			{
				return WikimediaLexem( WikimediaLexem::Timestamp, 0, timestmp);
			}
			else
			{
				++m_si;
			}
		}
		else if (isBibRefCandidate( m_si, m_se))
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			std::string bibref = tryParseBibRef();
			if (!bibref.empty())
			{
				return WikimediaLexem( WikimediaLexem::BibRef, 0, bibref);
			}
			else
			{
				++m_si;
			}
		}
		else if (isBookRefCandidate( m_si, m_se))
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			std::string bibref = tryParseBookRef();
			if (!bibref.empty())
			{
				return WikimediaLexem( WikimediaLexem::BibRef, 0, bibref);
			}
			else
			{
				++m_si;
			}
		}
		else if (isIsbnRefCandidate( m_si, m_se))
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			std::string bibref = tryParseIsbnRef();
			if (!bibref.empty())
			{
				return WikimediaLexem( WikimediaLexem::BibRef, 0, bibref);
			}
			else
			{
				++m_si;
			}
		}
		else if (isBigHexNumCandidate( m_si, m_se))
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			std::string bighexnum = tryParseBigHexNum();
			if (!bighexnum.empty())
			{
				return WikimediaLexem( WikimediaLexem::Code, 0, bighexnum);
			}
			else
			{
				++m_si;
			}
		}
		else if (isCodeCandidate( m_si, m_se))
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			std::string code = tryParseCode();
			if (!code.empty())
			{
				return WikimediaLexem( WikimediaLexem::Code, 0, code);
			}
			else
			{
				++m_si;
			}
		}
		else if (m_si < m_se && *m_si == '/')
		{
			const char* si_bk = m_si;
			char const* ti = m_si;
			while (m_si - ti < 15 &&  ti > start && isAlpha(*(ti-1))) --ti;
			if (m_si - ti < 15 && ti >= start)
			{
				m_si = ti;
				std::string filepath = tryParseFilePath();
				if (filepath.empty())
				{
					m_si = si_bk + 1;
				}
				else if (ti > start)
				{
					m_si = ti;
					return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, ti - start));
				}
				else
				{
					return WikimediaLexem( WikimediaLexem::Url, 0, std::string("file:") + filepath);
				}
			}
			else
			{
				++m_si;
			}
		}
		else if (m_si+1 < m_se && m_si[0] == '&' && isIdentifierChar( m_si[1], true/*with dash*/))
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			const char* paramend = skipUrlParameters( m_si, m_se);
			if (paramend)
			{
				const char* paramstart = m_si;
				m_si = paramend;
				return WikimediaLexem( WikimediaLexem::NoData, 0, std::string( paramstart, paramend - paramstart));
			}
			else
			{
				++m_si;
			}
		}
		else if (m_si < m_se && !isSpace(*m_si) && isRepPatternCandidate( m_si, m_se))
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, 0, std::string( start, m_si - start));
			}
			std::string ptstr = tryParseRepPattern();
			if (!ptstr.empty())
			{
				return WikimediaLexem( WikimediaLexem::NoData, 0, ptstr);
			}
			else
			{
				++m_si;
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

std::string WikimediaLexer::currentSourceExtract( int maxlen) const
{
	return outputLineString( m_prev_si, m_si+maxlen, maxlen);
}

std::string WikimediaLexer::rest() const
{
	return std::string( m_si, m_se-m_si);
}


