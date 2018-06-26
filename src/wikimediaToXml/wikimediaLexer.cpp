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
#include <string>
#include <vector>
#include <utility>
#include <cstring>
#include <stdexcept>

#undef STRUS_LOWLEVEL_DEBUG

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
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr<<"WARNING "<< "unclosed tag '" << tagname << "': " << outputString( si, se) << std::endl;
#endif
		
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
	const char* start = si;
#endif
	if (*si != '<') throw std::runtime_error("logic error: invalid call of open tag");
	si++;
	if (si < se && si[0] == '!' && si[1] == '-' && si[2] == '-')
	{
		const char* end = findPattern( si+2, se, "-->");
		if (!end)
		{
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "comment not closed:" << outputString( start, se) << std::endl;
#endif
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
	if (ti < m_se && *ti == assignop)
	{
		m_si = ++ti;
		return trim(rt);
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

bool WikimediaLexer::eatFollowChar( char expectChr)
{
	if (m_si < m_se && *m_si == expectChr)
	{
		++m_si;
		return true;
	}
	return false;
}

static char const* skipStyle( char const* si, char const* se)
{
	int sidx=0;
	char const* start = si;
	for (;si < se && *si > 0 && *si <= 32;++si){}
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
				return skipStyle( si+1, se);
			}
		}
		else if (si < se && isAlphaNum(*si))
		{
			for (;si < se && isAlphaNum(*si);++si){}
			if (si < se)
			{
				return skipStyle( si, se);
			}
		}
	}
	return start;
}

WikimediaLexem WikimediaLexer::next()
{
	m_prev_si = m_si;
	const char* start = m_si;
	while (m_si < m_se)
	{
		if (*m_si == '<')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, std::string( start, m_si - start));
			}
			m_si = skipTag( m_si, m_se);
			return WikimediaLexem( WikimediaLexem::Text, std::string( " ", 1));
		}
		else if (*m_si == '#')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, std::string( start, m_si - start));
			}
			if (0==std::memcmp( "#REDIRECT", m_si, 9))
			{
				m_si += 9;
				return WikimediaLexem( WikimediaLexem::Redirect);
			}
			else
			{
				++m_si;
			}
		}
		else if (*m_si == '[')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, std::string( start, m_si - start));
			}
			++m_si;
			if (m_si >= m_se) break;

			if (*m_si == '[')
			{
				++m_si;
				if (m_si < m_se && *m_si == '#')++m_si;
				std::string name = tryParseIdentifier( ':');
				return WikimediaLexem( WikimediaLexem::OpenLink, name);
			}
			else if (isAlpha(*m_si) || isSpace(*m_si))
			{
				std::string linkid = tryParseURL();
				if (linkid.size())
				{
					return WikimediaLexem( WikimediaLexem::OpenWWWLink, linkid);
				}
				else
				{
					return WikimediaLexem( WikimediaLexem::Text, " [");
				}
			}
			else
			{
				return WikimediaLexem( WikimediaLexem::Text, " [");
			}
		}
		else if (*m_si == '{')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, std::string( start, m_si - start));
			}
			++m_si;
			if (m_si >= m_se) break;

			if (*m_si == '{')
			{
				++m_si;
				std::string name = tryParseIdentifier( '|');
				return WikimediaLexem( WikimediaLexem::OpenCitation, name);
			}
			else if (*m_si == '|')
			{
				m_si = skipStyle( m_si, m_se);
				return WikimediaLexem( WikimediaLexem::OpenTable);
			}
		}
		else if (*m_si == '|')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, std::string( start, m_si - start) + " ");
			}
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
			else
			{
				m_si = skipStyle( m_si, m_se);
				return WikimediaLexem( WikimediaLexem::ColDelim);
			}
		}
		else if (*m_si == '!' && m_si+1 < m_se && m_si[1] == '!')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, std::string( start, m_si - start));
			}
			m_si += 2;
			m_si = skipStyle( m_si, m_se);
			return WikimediaLexem( WikimediaLexem::TableHeadDelim);
		}
		else if (*m_si == '\n')
		{
			while (m_si < m_se && isSpace(*m_si)) ++m_si;
			if (m_si < m_se)
			{
				if (*m_si == '!')
				{
					if (start < m_si-1)
					{
						--m_si;
						return WikimediaLexem( WikimediaLexem::Text, std::string( start, m_si - start));
					}
					++m_si;
					m_si = skipStyle( m_si, m_se);
					return WikimediaLexem( WikimediaLexem::TableHeadDelim);
				}
				else if (*m_si == '|')
				{
					continue;
				}
				else if (*m_si == '*')
				{
					if (start < m_si-1)
					{
						--m_si;
						return WikimediaLexem( WikimediaLexem::Text, std::string( start, m_si - start));
					}
					++m_si;
					int lidx = 0;
					for (; m_si != m_se && *m_si == '*'; ++lidx,++m_si){}
					if (lidx >= 4) lidx = 3;
					return WikimediaLexem( (WikimediaLexem::Id)(WikimediaLexem::ListItem1 + lidx));
				}
				else if (*m_si == '=')
				{
					if (start < m_si-1)
					{
						--m_si;
						return WikimediaLexem( WikimediaLexem::Text, trim( std::string( start, m_si - start)));
					}
					char const* ti = ++m_si;
					int tcnt = 0;
					for (; ti != m_se && *ti == '='; ++tcnt,++ti){}
					if (tcnt <= 5 && tcnt >= 1)
					{
						start = ti;
						for (; ti < m_se && *ti != '=' && *ti != '\n'; ++ti){}
						if (ti < m_se && *ti == '=')
						{
							const char* end = m_si = ti;
							for (; m_si != m_se && *m_si == '='; ++m_si){}
							return WikimediaLexem( (WikimediaLexem::Id)(WikimediaLexem::Heading1+(tcnt-1)), std::string( start, end-start));
						}
					}
				}
				else if (m_si+6 < m_se
					&& (isEqual("File:",m_si,5) || isEqual("Image:",m_si,6)))
				{
					m_si += 6;
					for (; m_si < m_se && *m_si != '\n' && *m_si != '|'; ++m_si){}
					if (m_si < m_se && *m_si == '|')
					{
						++m_si;
						return WikimediaLexem( WikimediaLexem::ListItem1);
					}
				}
				else
				{
					if (start < m_si-1)
					{
						--m_si;
						return WikimediaLexem( WikimediaLexem::Text, std::string( start, m_si - start));
					}
					return WikimediaLexem( WikimediaLexem::EndOfLine);
				}
			}
		}
		else if (*m_si == '}' && m_si+1 < m_se && m_si[1] == '}')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, std::string( start, m_si - start));
			}
			m_si += 2;
			return WikimediaLexem( WikimediaLexem::CloseCitation);
		}
		else if (*m_si == ']')
		{
			if (start != m_si)
			{
				return WikimediaLexem( WikimediaLexem::Text, std::string( start, m_si - start));
			}
			++m_si;
			if (m_si < m_se && *m_si == ']')
			{
				++m_si;
				return WikimediaLexem( WikimediaLexem::CloseLink);
			}
			else
			{
				return WikimediaLexem( WikimediaLexem::Text, "] ");
			}
		}
		else
		{
			++m_si;
		}
	}
	if (start != m_si)
	{
		return WikimediaLexem( WikimediaLexem::Text, std::string( start, m_si - start));
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


