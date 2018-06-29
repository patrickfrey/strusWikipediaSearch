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
		Error,
		Text,
		Math,
		NoWiki,
		Url,
		Redirect,
		OpenHeading,
		CloseHeading,
		OpenRef,
		CloseRef,
		ListItem,
		EndOfLine,
		EntityMarker,
		QuotationMarker,
		OpenCitation,
		CloseCitation,
		OpenWWWLink,
		CloseWWWLink,
		OpenPageLink,
		ClosePageLink,
		OpenTable,
		CloseTable,
		TableTitle,
		TableRowDelim,
		TableColDelim,
		TableHeadDelim,
		ColDelim
	};
	static const char* idName( Id lexemId)
	{
		static const char* ar[] =
		{
			"EOF","Error","Text","URL","Redirect","OpenHeading","CloseHeading","ListItem",
			"EndOfLine","EntityMarker","QuotationMarker","OpenCitation","CloseCitation",
			"OpenWWWLink","CloseWWWLink","OpenPageLink","ClosePageLink",
			"OpenTable", "CloseTable","TableTitle","TableRowDelim","TableColDelim",
			"TableHeadDelim","ColDelim",0
		};
		return ar[lexemId];
	}

	WikimediaLexem( Id id_, int idx_, const std::string& value_)
		:id(id_),idx(idx_),value(value_){}
	WikimediaLexem( Id id_)
		:id(id_),idx(0),value(){}
	WikimediaLexem( const WikimediaLexem& o)
		:id(o.id),idx(o.idx),value(o.value){}

	Id id;
	int idx;
	std::string value;
};

class WikimediaLexer
{
public:

	WikimediaLexer( const char* src, std::size_t size)
		:m_prev_si(src),m_si(src),m_se(src+size),m_curHeading(0){}

	WikimediaLexem next();
	std::string rest() const;
	std::string currentSourceExtract() const;
	void unget()					{m_si = m_prev_si;}

private:
	std::string tryParseIdentifier( char assignop);
	std::string tryParseURL();
	std::string tryParseLinkId();
	bool eatFollowChar( char expectChr);

private:
	char const* m_prev_si;
	char const* m_si;
	const char* m_se;
	int m_curHeading;
};

}//namespace
#endif

