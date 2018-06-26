/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/// \brief Lexer for Wikimedia format
/// \file documentStructure.hpp
#ifndef _STRUS_WIKIPEDIA_WIKIMEDIA_LEXER_HPP_INCLUDED
#define _STRUS_WIKIPEDIA_WIKIMEDIA_LEXER_HPP_INCLUDED
#include <string>
#include <vector>
#include <utility>

/// \brief strus toplevel namespace
namespace strus {

struct WikimediaLexem
{
	enum Id
	{
		EoF,
		Text,
		Redirect,
		Heading1,
		Heading2,
		Heading3,
		Heading4,
		Heading5,
		Heading6,
		ListItem1,
		ListItem2,
		ListItem3,
		ListItem4,
		EndOfLine,
		OpenCitation,
		CloseCitation,
		OpenWWWLink,
		OpenLink,
		CloseLink,
		OpenTable,
		CloseTable,
		TableRowDelim,
		TableTitle,
		TableHeadDelim,
		ColDelim
	};
	static const char* idName( Id lexemId)
	{
		static const char* ar[] =
		{
			"EOF","Text","Redirect","Heading1","Heading2","Heading3","Heading4",
			"Heading5","Heading6","ListItem1","ListItem2","ListItem3","ListItem4",
			"EndOfLine","OpenCitation","CloseCitation","OpenWWWLink", "OpenSquareBracket",
			"CloseSquareBracket","OpenLink","CloseLink",
			"OpenTable", "CloseTable","TableRowDelim","TableTitle",
			"TableHeadDelim","ColDelim",0
		};
		return ar[lexemId];
	}

	WikimediaLexem( Id id_, const std::string& value_)
		:id(id_),value(value_){}
	WikimediaLexem( Id id_)
		:id(id_),value(){}
	WikimediaLexem( const WikimediaLexem& o)
		:id(o.id),value(o.value){}

	Id id;
	std::string value;
};

class WikimediaLexer
{
public:

	WikimediaLexer( const char* src, std::size_t size)
		:m_prev_si(src),m_si(src),m_se(src+size){}

	WikimediaLexem next();
	std::string rest() const;
	std::string currentSourceExtract() const;
	void unget()					{m_si = m_prev_si;}

private:
	std::string tryParseIdentifier( char assignop);
	std::string tryParseURL();
	bool eatFollowChar( char expectChr);

private:
	char const* m_prev_si;
	char const* m_si;
	const char* m_se;
};

}//namespace
#endif

