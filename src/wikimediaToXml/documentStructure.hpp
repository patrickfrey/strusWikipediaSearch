/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/// \brief Intermediate document format for converting "Wikimedia XML" to pure and simplified XML
/// \file documentStructure.hpp
#ifndef _STRUS_WIKIPEDIA_DOCUMENT_STRUCTURE_HPP_INCLUDED
#define _STRUS_WIKIPEDIA_DOCUMENT_STRUCTURE_HPP_INCLUDED
#include <string>
#include <map>
#include <vector>
#include <utility>

/// \brief strus toplevel namespace
namespace strus {

class Paragraph
{
public:
	enum Type {
			Title,
			EntityStart,
			EntityEnd,
			QuotationStart,
			QuotationEnd,
			HeadingStart,
			HeadingEnd,
			ListItemStart,
			ListItemEnd,
			CitationStart,
			CitationEnd,
			RefStart,
			RefEnd,
			TableStart,
			TableEnd,
			TableTitleStart,
			TableTitleEnd,
			TableHeadStart,
			TableHeadEnd,
			TableRowStart,
			TableRowEnd,
			TableColStart,
			TableColEnd,
			Attribute,
			Text,
			NoWiki,
			Math,
			PageLink,
			WebLink,
			CitationLink
	};
	static const char* typeName( Type tp)
	{
		static const char* ar[] = {
			"Title",
			"EntityStart",
			"EntityEnd",
			"QuotationStart",
			"QuotationEnd",
			"HeadingStart",
			"HeadingEnd",
			"ListItemStart",
			"ListItemEnd",
			"CitationStart",
			"CitationEnd",
			"RefStart",
			"RefEnd",
			"TableStart",
			"TableEnd",
			"TableTitleStart",
			"TableTitleEnd",
			"TableHeadStart",
			"TableHeadEnd",
			"TableRowStart",
			"TableRowEnd",
			"TableColStart",
			"TableColEnd",
			"Attribute",
			"Text",
			"NoWiki",
			"Math",
			"PageLink",
			"WebLink",
			"CitationLink",0};
		return ar[tp];
	}
	enum StructType {StructNone,StructEntity,StructQuotation,StructHeading,StructList,StructCitation,StructRef,StructTable,StructTableTitle,StructTableHead,StructTableRow,StructTableCol};
	static const char* structTypeName( StructType st)
	{
		static const char* ar[] = {"None","Entity","Quotation","Heading","List","Citation","Ref","Table","TableTitle","TableHead","TableRow","TableCol"};
		return ar[ st];
	}
	static StructType structType( Type ti)
	{
		static StructType ar[] = {
			StructNone/*Title*/,
			StructEntity/*EntityStart*/,
			StructNone/*EntityEnd*/,
			StructQuotation/*QuotationStart*/,
			StructNone/*QuotationEnd*/,
			StructHeading/*HeadingStart*/,
			StructNone/*HeadingEnd*/,
			StructList/*ListItemStart*/,
			StructNone/*ListItemEnd*/,
			StructCitation/*CitationStart*/,
			StructNone/*CitationEnd*/,
			StructRef/*RefStart*/,
			StructNone/*RefEnd*/,
			StructTable/*TableStart*/,
			StructNone/*TableEnd*/,
			StructTableTitle/*TableTitleStart*/,
			StructNone/*TableTitleEnd*/,
			StructTableHead/*TableHeadStart*/,
			StructNone/*TableHeadEnd*/,
			StructTableRow/*TableRowStart*/,
			StructNone/*TableRowEnd*/,
			StructTableCol/*TableColStart*/,
			StructNone/*TableColEnd*/,
			StructNone/*Attribute*/,
			StructNone/*Text*/,
			StructNone/*NoWiki*/,
			StructNone/*Math*/,
			StructNone/*PageLink*/,
			StructNone/*WebLink*/,
			StructNone/*CitationLink*/};
		return ar[ti];
	}
	static Type invType( Type ti)
	{
		static Type ar[] = {
			Title/*Title*/,
			EntityEnd/*EntityStart*/,
			EntityStart/*EntityEnd*/,
			QuotationEnd/*QuotationStart*/,
			QuotationStart/*QuotationEnd*/,
			HeadingEnd/*HeadingStart*/,
			HeadingStart/*HeadingEnd*/,
			ListItemEnd/*ListItemStart*/,
			ListItemStart/*ListItemEnd*/,
			CitationEnd/*CitationStart*/,
			CitationStart/*CitationEnd*/,
			RefEnd/*RefStart*/,
			RefStart/*RefEnd*/,
			TableEnd/*TableStart*/,
			TableStart/*TableEnd*/,
			TableTitleEnd/*TableTitleStart*/,
			TableTitleStart/*TableTitleEnd*/,
			TableHeadEnd/*TableHeadStart*/,
			TableHeadStart/*TableHeadEnd*/,
			TableRowEnd/*TableRowStart*/,
			TableRowStart/*TableRowEnd*/,
			TableColEnd/*TableColStart*/,
			TableColStart/*TableColEnd*/,
			Attribute/*Attribute*/,
			Text/*Text*/,
			NoWiki/*NoWiki*/,
			Math/*Math*/,
			PageLink/*PageLink*/,
			WebLink/*WebLink*/,
			CitationLink/*CitationLink*/};
		return ar[ti];
	}
	static bool structTypeIsAutoCloseOnEoln( StructType tp)
	{
		return (tp == StructList || tp == StructTableCol || tp == StructTableRow || tp == StructEntity);
	}

	StructType structType() const
	{
		return structType( m_type);
	}
	const char* typeName() const
	{
		return typeName( m_type);
	}
	const char* structTypeName() const
	{
		return structTypeName( structType( m_type));
	}

	Paragraph()
		:m_type(Text),m_id(),m_text(){}
	Paragraph( Type type_, const std::string& id_, const std::string& text_)
		:m_type(type_),m_id(id_),m_text(text_){}
	Paragraph( const Paragraph& o)
		:m_type(o.m_type),m_id(o.m_id),m_text(o.m_text){}

	Type type() const					{return m_type;}
	const std::string& id() const				{return m_id;}
	const std::string& text() const				{return m_text;}

	void setId( const std::string& id_)
	{
		m_id = id_;
	}
	void setText( const std::string& text_)
	{
		m_text = text_;
	}
	void addText( const std::string& text_)
	{
		m_text += text_;
	}

private:
	Type m_type;
	std::string m_id;
	std::string m_text;
};


class DocumentStructure
{
public:
	explicit DocumentStructure()
		:m_id(),m_parar(),m_citations(),m_structStack(),m_tableDefs(),m_errors(),m_citationCnt(0),m_refCnt(0){}
	DocumentStructure( const DocumentStructure& o)
		:m_id(o.m_id),m_parar(o.m_parar),m_citations(o.m_citations),m_structStack(o.m_structStack),m_tableDefs(o.m_tableDefs),m_errors(o.m_errors),m_citationCnt(o.m_citationCnt),m_refCnt(o.m_refCnt){}

	const std::string& id() const
	{
		return m_id;
	}

	void setTitle( const std::string& text);
	void addText( const std::string& text)
	{
		addSingleItem( Paragraph::Text, "", text, true/*joinText*/);
	}
	void addMath( const std::string& text)
	{
		addSingleItem( Paragraph::Math, "", text, false/*joinText*/);
	}
	void addNoWiki( const std::string& text)
	{
		addSingleItem( Paragraph::NoWiki, "", text, false/*joinText*/);
	}
	void openRef()
	{
		openStructure( Paragraph::RefStart, "ref", ++m_refCnt);
	}
	void closeRef()
	{
		closeStructure( Paragraph::RefStart);
	}
	void openHeading( int idx)
	{
		closeOpenStructures();
		openAutoCloseItem( Paragraph::HeadingStart, "h", idx);
	}
	void closeHeading()
	{
		closeAutoCloseItem( Paragraph::HeadingStart);
	}
	void addPageLink( const std::string& id, const std::string& text)
	{
		addSingleItem( Paragraph::PageLink, id, text, false/*joinText*/);
	}
	void addWebLink( const std::string& id, const std::string& text)
	{
		addSingleItem( Paragraph::WebLink, id, text, false/*joinText*/);
	}
	void addEntityMarker()
	{
		addQuoteItem( Paragraph::EntityStart);
	}
	void addQuotationMarker()
	{
		addQuoteItem( Paragraph::QuotationStart);
	}
	void openTable()
	{
		closeDanglingStructures( Paragraph::TableStart);
		openStructure( Paragraph::TableStart, "table", (int)m_tableDefs.size()+1);
	}
	void closeTable()
	{
		closeStructure( Paragraph::TableStart);
	}
	void addTableTitle()
	{
		closeDanglingStructures( Paragraph::TableStart);
		openAutoCloseItem( Paragraph::TableTitleStart, "table", (int)m_tableDefs.size());
	}
	void addTableHead()
	{
		closeDanglingStructures( Paragraph::TableStart);
		if (m_tableDefs.empty()) {addError("table open head without table defined"); return;}
		openAutoCloseItem( Paragraph::TableHeadStart, "head", ++m_tableDefs.back().headitr);
	}
	void addTableRow()
	{
		closeDanglingStructures( Paragraph::TableStart);
		if (m_tableDefs.empty()) {addError("table open row without table defined"); return;}
		m_tableDefs.back().coliter = 0;
		openAutoCloseItem( Paragraph::TableTitleStart, "row", ++m_tableDefs.back().rowiter);
	}
	void addTableCol()
	{
		closeDanglingStructures( Paragraph::TableRowStart);
		if (m_tableDefs.empty()) {addError("table open col without table defined"); return;}
		openAutoCloseItem( Paragraph::TableTitleStart, "col", ++m_tableDefs.back().coliter);
	}
	void addAttribute( const std::string& id, const std::string& text)
	{
		if (currentStructType() != Paragraph::StructCitation) {addError("add attribute without context"); return;}
		addSingleItem( Paragraph::Attribute, id, text, false/*joinText*/);
	}
	void openListItem( int lidx)
	{
		openAutoCloseItem( Paragraph::ListItemStart, "l", lidx);
	}
	void closeOpenEolnItem()
	{
		closeAutoCloseItem( Paragraph::ListItemStart);
		closeAutoCloseItem( Paragraph::HeadingStart);
	}
	void openCitation()
	{
		openStructure( Paragraph::CitationStart, "cit", ++m_citationCnt);
	}

	void closeCitation()
	{
		closeStructure( Paragraph::CitationStart);
	}

	void addError( const std::string& msg)
	{
		m_errors.push_back( msg);
	}

	const std::vector<std::string>& errors() const
	{
		return m_errors;
	}
	void finish();

	std::string toxml( bool beautified) const;
	std::string tostring() const;
	std::string statestring() const;

private:
	void finishStructure( int structStartidx);
	void openStructure( Paragraph::Type startType, const char* prefix, int lidx=0);
	void closeStructure( Paragraph::Type startType);
	void closeOpenStructures();

	void addSingleItem( Paragraph::Type type, const std::string& id, const std::string& text, bool joinText);
	void addQuoteItem( Paragraph::Type startType);
	void openAutoCloseItem( Paragraph::Type startType, const char* prefix, int lidx);
	void closeAutoCloseItem( Paragraph::Type startType);
	void closeDanglingStructures( const Paragraph::Type& starttype);
	void checkStartEndSectionBalance( const std::vector<Paragraph>::const_iterator& start, const std::vector<Paragraph>::const_iterator& end);
	Paragraph::StructType currentStructType() const;

private:
	struct StructRef
	{
		int idx;
		int start;

		StructRef( int idx_, int start_)
			:idx(idx_),start(start_){}
		StructRef( const StructRef& o)
			:idx(o.idx),start(o.start){}
	};
	struct TableDef
	{
		int headitr;
		int rowiter;
		int coliter;
		int start;

		explicit TableDef( int start_)
			:headitr(0),rowiter(0),coliter(0),start(start_){}
		TableDef( const TableDef& o)
			:headitr(o.headitr),rowiter(o.rowiter),coliter(o.coliter),start(o.start){}
	};

private:
	std::string m_id;
	std::vector<Paragraph> m_parar;
	std::vector<Paragraph> m_citations;
	std::vector<StructRef> m_structStack;
	std::vector<TableDef> m_tableDefs;
	std::vector<std::string> m_errors;
	int m_citationCnt;
	int m_refCnt;
};

}//namespace
#endif

