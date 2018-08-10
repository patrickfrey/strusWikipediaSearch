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
#include "strus/base/string_format.hpp"
#include "strus/base/fileio.hpp"
#include <string>
#include <map>
#include <set>
#include <vector>
#include <utility>
#include <cstring>
#include <sstream>
#include <iostream>

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
			DoubleQuoteStart,
			DoubleQuoteEnd,
			BlockQuoteStart,
			BlockQuoteEnd,
			DivStart,
			DivEnd,
			PoemStart,
			PoemEnd,
			SpanStart,
			SpanEnd,
			FormatStart,
			FormatEnd,
			HeadingStart,
			HeadingEnd,
			ListItemStart,
			ListItemEnd,
			AttributeStart,
			AttributeEnd,
			CitationStart,
			CitationEnd,
			RefStart,
			RefEnd,
			PageLinkStart,
			PageLinkEnd,
			WebLinkStart,
			WebLinkEnd,
			TableStart,
			TableEnd,
			TableTitleStart,
			TableTitleEnd,
			TableHeadStart,
			TableHeadEnd,
			TableCellStart,
			TableCellEnd,
			TableCellReference,
			Markup,
			Text,
			Char,
			BibRef,
			NoWiki,
			Code,
			Math,
			Timestamp,
			CitationLink,
			RefLink,
			TableLink
	};
	static const char* typeName( Type tp)
	{
		static const char* ar[] = {
			"Title",
			"EntityStart",
			"EntityEnd",
			"QuotationStart",
			"QuotationEnd",
			"DoubleQuoteStart",
			"DoubleQuoteEnd",
			"BlockQuoteStart",
			"BlockQuoteEnd",
			"DivStart",
			"DivEnd",
			"PoemStart",
			"PoemEnd",
			"SpanStart",
			"SpanEnd",
			"FormatStart",
			"FormatEnd",
			"HeadingStart",
			"HeadingEnd",
			"ListItemStart",
			"ListItemEnd",
			"AttributeStart",
			"AttributeEnd",
			"CitationStart",
			"CitationEnd",
			"RefStart",
			"RefEnd",
			"PageLinkStart",
			"PageLinkEnd",
			"WebLinkStart",
			"WebLinkEnd",
			"TableStart",
			"TableEnd",
			"TableTitleStart",
			"TableTitleEnd",
			"TableHeadStart",
			"TableHeadEnd",
			"TableCellStart",
			"TableCellEnd",
			"TableCellReference",
			"Markup",
			"Text",
			"Char",
			"BibRef",
			"NoWiki",
			"Code",
			"Math",
			"Timestamp",
			"CitationLink",
			"RefLink",
			"TableLink", 0};
		return ar[tp];
	}
	enum StructType {
			StructNone,
			StructEntity,
			StructQuotation,
			StructDoubleQuote,
			StructBlockQuote,
			StructDiv,
			StructPoem,
			StructSpan,
			StructFormat,
			StructHeading,
			StructList,
			StructAttribute,
			StructCitation,
			StructRef,
			StructPageLink,
			StructWebLink,
			StructTable,
			StructTableTitle,
			StructTableHead,
			StructTableCell
	};
	static const char* structTypeName( StructType st)
	{
		static const char* ar[] = {"None","Entity","Quotation","DoubleQuote","BlockQuote","Div","Poem","Span","Format","Heading","List","Attribute","Citation","Ref","PageLink","WebLink","Table","TableTitle","TableHead","TableCell",0};
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
			StructDoubleQuote/*DoubleQuoteStart*/,
			StructNone/*DoubleQuoteEnd*/,
			StructBlockQuote/*BlockQuoteStart*/,
			StructNone/*BlockQuoteEnd*/,
			StructDiv/*DivStart*/,
			StructNone/*DivEnd*/,
			StructPoem/*BlockPoem*/,
			StructNone/*BlockPoem*/,
			StructSpan/*SpanStart*/,
			StructNone/*SpanEnd*/,
			StructFormat/*FormatStart*/,
			StructNone/*FormatEnd*/,
			StructHeading/*HeadingStart*/,
			StructNone/*HeadingEnd*/,
			StructList/*ListItemStart*/,
			StructNone/*ListItemEnd*/,
			StructAttribute/*AttributeStart*/,
			StructNone/*AttributeEnd*/,
			StructCitation/*CitationStart*/,
			StructNone/*CitationEnd*/,
			StructRef/*RefStart*/,
			StructNone/*RefEnd*/,
			StructPageLink/*PageLinkStart*/,
			StructNone/*PageLinkEnd*/,
			StructWebLink/*WebLinkStart*/,
			StructNone/*WebLinkEnd*/,
			StructTable/*TableStart*/,
			StructNone/*TableEnd*/,
			StructTableTitle/*TableTitleStart*/,
			StructNone/*TableTitleEnd*/,
			StructTableHead/*TableHeadStart*/,
			StructNone/*TableHeadEnd*/,
			StructTableCell/*TableCellStart*/,
			StructNone/*TableCellEnd*/,

			StructNone/*TableCellReference*/,
			StructNone/*Markup*/,
			StructNone/*Text*/,
			StructNone/*Char*/,
			StructNone/*BibRef*/,
			StructNone/*NoWiki*/,
			StructNone/*Code*/,
			StructNone/*Math*/,
			StructNone/*Timestamp*/,
			StructNone/*CitationLink*/,
			StructNone/*RefLink*/,
			StructNone/*TableLink*/};
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
			DoubleQuoteEnd/*DoubleQuoteStart*/,
			DoubleQuoteStart/*DoubleQuoteEnd*/,
			BlockQuoteEnd/*BlockQuoteStart*/,
			BlockQuoteStart/*BlockQuoteEnd*/,
			DivEnd/*DivStart*/,
			DivStart/*DivEnd*/,
			PoemEnd/*PoemStart*/,
			PoemStart/*PoemEnd*/,
			SpanEnd/*SpanStart*/,
			SpanStart/*SpanEnd*/,
			FormatEnd/*FormatStart*/,
			FormatStart/*FormatEnd*/,
			HeadingEnd/*HeadingStart*/,
			HeadingStart/*HeadingEnd*/,
			ListItemEnd/*ListItemStart*/,
			ListItemStart/*ListItemEnd*/,
			AttributeEnd/*AttributeStart*/,
			AttributeStart/*AttributeEnd*/,
			CitationEnd/*CitationStart*/,
			CitationStart/*CitationEnd*/,
			RefEnd/*RefStart*/,
			RefStart/*RefEnd*/,
			PageLinkEnd/*PageLinkStart*/,
			PageLinkStart/*PageLinkEnd*/,
			WebLinkEnd/*WebLinkStart*/,
			WebLinkStart/*WebLinkEnd*/,
			TableEnd/*TableStart*/,
			TableStart/*TableEnd*/,
			TableTitleEnd/*TableTitleStart*/,
			TableTitleStart/*TableTitleEnd*/,
			TableHeadEnd/*TableHeadStart*/,
			TableHeadStart/*TableHeadEnd*/,
			TableCellEnd/*TableCellStart*/,
			TableCellStart/*TableCellEnd*/,

			TableCellReference/*TableCellReference*/,
			Markup/*Markup*/,
			Text/*Text*/,
			Char/*Char*/,
			BibRef/*BibRef*/,
			NoWiki/*NoWiki*/,
			Code/*Code*/,
			Math/*Math*/,
			Timestamp/*Timestamp*/,
			CitationLink/*CitationLink*/,
			RefLink/*RefLink*/,
			TableLink/*TableLink*/
			};
		return ar[ti];
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
	std::string tokey() const
	{
		return strus::string_format( "%d\1%s\1%s", (int)m_type, m_id.c_str(), m_text.c_str());
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
		:m_fileId(),m_parar(),m_citations(),m_tables(),m_refs(),m_citationmap(),m_refmap(),m_structStack(),m_tableDefs(),m_errors(),m_nofErrors(0),m_tableCnt(0),m_citationCnt(0),m_refCnt(0),m_lastHeadingIdx(0){}
	DocumentStructure( const DocumentStructure& o)
		:m_fileId(o.m_fileId),m_parar(o.m_parar),m_citations(o.m_citations),m_tables(o.m_tables),m_refs(o.m_refs),m_citationmap(o.m_citationmap),m_refmap(o.m_refmap),m_structStack(o.m_structStack),m_tableDefs(o.m_tableDefs),m_errors(o.m_errors),m_nofErrors(o.m_nofErrors),m_tableCnt(o.m_tableCnt),m_citationCnt(o.m_citationCnt),m_refCnt(o.m_refCnt),m_lastHeadingIdx(o.m_lastHeadingIdx){}

	const std::string& fileId() const
	{
		return m_fileId;
	}
	Paragraph::StructType currentStructType() const;
	int currentStructIndex() const;

	void setTitle( const std::string& text);

	void addMarkup( const std::string& text)
	{
		addSingleItem( Paragraph::Markup, "", text, false/*joinText*/);
	}
	void addText( const std::string& text)
	{
		addSingleItem( Paragraph::Text, "", text, true/*joinText*/);
	}
	void addChar( const std::string& text)
	{
		addSingleItem( Paragraph::Char, "", text, false/*joinText*/);
	}
	void clearOpenText();

	void addMath( const std::string& text)
	{
		addSingleItem( Paragraph::Math, "", text, false/*joinText*/);
	}
	void addNoWiki( const std::string& text)
	{
		addSingleItem( Paragraph::NoWiki, "", text, false/*joinText*/);
	}
	void addCode( const std::string& text)
	{
		addSingleItem( Paragraph::Code, "", text, false/*joinText*/);
	}
	void addTimestamp( const std::string& text)
	{
		addSingleItem( Paragraph::Timestamp, "", text, false/*joinText*/);
	}
	void addBibRef( const std::string& text)
	{
		addSingleItem( Paragraph::BibRef, "", text, false/*joinText*/);
	}
	void openRef()
	{
		closeOpenQuoteItems();
		closeAutoCloseItem( Paragraph::RefStart);
		openStructure( Paragraph::RefStart, "ref", ++m_refCnt);
	}
	void closeRef()
	{
		closeStructure( Paragraph::RefStart, "");
	}
	void openPageLink( const std::string& pageid, const std::string& anchorid)
	{
		openStructure( Paragraph::PageLinkStart, pageid.c_str(), 0);
		if (!anchorid.empty())
		{
			m_parar.push_back( Paragraph( Paragraph::AttributeStart, "anchor", anchorid));
			m_parar.push_back( Paragraph( Paragraph::AttributeEnd, "", ""));
		}
	}
	void closePageLink()
	{
		closeStructure( Paragraph::PageLinkStart, "");
	}
	void openWebLink( const std::string& id)
	{
		openStructure( Paragraph::WebLinkStart, id.c_str(), 0);
	}
	void closeWebLink();

	void openHeading( int idx)
	{
		m_lastHeadingIdx = idx;
		closeOpenStructures();
		openAutoCloseItem( Paragraph::HeadingStart, "h", idx);
	}
	void addHeadingItem()
	{
		closeOpenStructures();
		openAutoCloseItem( Paragraph::HeadingStart, "h", m_lastHeadingIdx+1);
	}
	void closeHeading()
	{
		closeAutoCloseItem( Paragraph::HeadingStart);
	}
	void addEntityMarker()
	{
		addQuoteItem( Paragraph::EntityStart);
	}
	void addQuotationMarker()
	{
		addQuoteItem( Paragraph::QuotationStart);
	}
	void addDoubleQuoteMarker()
	{
		addQuoteItem( Paragraph::DoubleQuoteStart);
	}
	void openDoubleQuote()
	{
		closeOpenQuoteItems();
		openStructure( Paragraph::DoubleQuoteStart, "", 0);
	}
	void closeDoubleQuote()
	{
		closeStructure( Paragraph::DoubleQuoteStart, "");
	}
	void openBlockQuote()
	{
		closeOpenQuoteItems();
		closeAutoCloseItem( Paragraph::BlockQuoteStart);
		openStructure( Paragraph::BlockQuoteStart, "", 0);
	}
	void closeBlockQuote()
	{
		closeStructure( Paragraph::BlockQuoteStart, "");
	}
	void openDiv()
	{
		closeOpenQuoteItems();
		openStructure( Paragraph::DivStart, "", 0);
	}
	void closeDiv()
	{
		closeStructure( Paragraph::DivStart, "");
	}
	void openPoem()
	{
		closeOpenQuoteItems();
		closeAutoCloseItem( Paragraph::PoemStart);
		openStructure( Paragraph::PoemStart, "", 0);
	}
	void closePoem()
	{
		closeStructure( Paragraph::PoemStart, "");
	}
	void openSpan()
	{
		closeOpenQuoteItems();
		closeAutoCloseItem( Paragraph::SpanStart);
		openStructure( Paragraph::SpanStart, "", 0);
	}
	void closeSpan()
	{
		closeStructure( Paragraph::SpanStart, "");
	}
	void openFormat()
	{
		closeOpenQuoteItems();
		openStructure( Paragraph::FormatStart, "", 0);
	}
	void closeFormat()
	{
		closeStructure( Paragraph::FormatStart, "");
	}
	void implicitOpenTableIfUndefined()
	{
		strus::Paragraph::StructType tp = currentStructType();

		if (tp != strus::Paragraph::StructTable
		&&  tp != strus::Paragraph::StructTableTitle
		&&  tp != strus::Paragraph::StructTableHead
		&&  tp != strus::Paragraph::StructTableCell)
		{
			openTable();
		}
	}
	void openTable()
	{
		closeOpenQuoteItems();
		closeDanglingStructures( Paragraph::TableStart);
		openStructure( Paragraph::TableStart, "table", ++m_tableCnt);
	}
	void closeTable()
	{
		if (m_tableDefs.empty())
		{
			addError("table close without table defined");
		}
		else
		{
			closeStructure( Paragraph::TableStart, "");
		}
	}
	bool closeOpenEmptyStruct( const Paragraph::Type& type)
	{
		if (currentStructType() == Paragraph::structType(type) && m_parar.back().type() == type)
		{
			m_parar.pop_back();
			m_structStack.pop_back();
			return true;
		}
		return false;
	}
	void addTableTitle()
	{
		if (!checkTableDefExists( "table add title")) return;
		closeDanglingStructures( Paragraph::TableStart);
		openAutoCloseItem( Paragraph::TableTitleStart, "title", 0);
	}
	void openTableCell( Paragraph::Type startType, int rowspan, int colspan);

	void addTableHead( int rowspan, int colspan)
	{
		if (!checkTableDefExists( "table add head")) return;
		closeDanglingStructures( Paragraph::TableStart);
		openTableCell( Paragraph::TableHeadStart, rowspan, colspan);
	}
	void repeatTableCell( int rowspan, int colspan)
	{
		strus::Paragraph::StructType tp = currentStructType();
		strus::Paragraph::Type reptype = (tp == Paragraph::StructTableHead) ? strus::Paragraph::TableHeadStart : strus::Paragraph::TableCellStart;

		if (!checkTableDefExists( "table repeat cell")) return;
		closeDanglingStructures( Paragraph::TableStart);
		openTableCell( reptype, rowspan, colspan);
	}
	void addTableCell( int rowspan, int colspan)
	{
		closeDanglingStructures( Paragraph::TableStart);
		openTableCell( Paragraph::TableCellStart, rowspan, colspan);
	}
	void addTableRow()
	{
		if (!checkTableDefExists( "table add head row")) return;
		closeDanglingStructures( Paragraph::TableStart);
		m_tableDefs.back().nextRow();
	}
	void addAttribute( const std::string& id)
	{
		closeAutoCloseItem( Paragraph::AttributeStart);
		Paragraph::StructType tp = currentStructType();
		if (tp == Paragraph::StructCitation
		||  tp == Paragraph::StructRef
		||  tp == Paragraph::StructAttribute)
		{
			openAutoCloseItem( Paragraph::AttributeStart, id.c_str(), 0);
		}
		else
		{
			addError( strus::string_format( "add attribute without context: '%s'", id.c_str()));
		}
	}
	void openListItem( int lidx)
	{
		closeOpenEolnItem();
		openAutoCloseItem( Paragraph::ListItemStart, "l", lidx);
	}
	void closeOpenEolnItem()
	{
		closeOpenQuoteItems();
		closeAutoCloseItem( Paragraph::ListItemStart);
		closeAutoCloseItem( Paragraph::HeadingStart);
	}
	void closeOpenQuoteItems();

	void openCitation( const std::string& citclass)
	{
		closeOpenQuoteItems();
		openStructure( Paragraph::CitationStart, "cit", ++m_citationCnt);
		if (!citclass.empty())
		{
			m_parar.push_back( Paragraph( Paragraph::AttributeStart, "class", citclass));
			m_parar.push_back( Paragraph( Paragraph::AttributeEnd, "", ""));
		}
	}

	void closeCitation()
	{
		closeAutoCloseItem( Paragraph::AttributeStart);
		closeStructure( Paragraph::CitationStart, "");
	}

	void addError( const std::string& msg)
	{
		m_errors.push_back( msg + " [state " + Paragraph::structTypeName( currentStructType()) + "]");
	}
	void addUnresolved( const std::string& pglink)
	{
		m_unresolved.insert( pglink);
	}
	bool hasNewErrors() const
	{
		return (int)m_errors.size() > m_nofErrors;
	}
	void setErrorsSourceInfo( const std::string& msg);

	const std::vector<std::string>& errors() const
	{
		return m_errors;
	}
	std::vector<std::string> unresolved() const
	{
		return std::vector<std::string>( m_unresolved.begin(), m_unresolved.end());
	}
	void finish();

	std::string toxml( bool beautified, bool singleIdAttribute) const;
	std::string tostring() const;
	std::string reportStrangeFeatures() const;
	std::string statestring() const;

	static std::string getInputXML( const std::string& title, const std::string& content);

private:
	void finishStructure( int structStartidx);
	void finishTable( int structStartidx, const std::string& id);
	void finishRef( int structStartidx, const std::string& id);
	void finishCitation( int startidx, const std::string& id);
	void openStructure( Paragraph::Type startType, const char* prefix, int lidx=0);
	void closeStructure( Paragraph::Type startType, const std::string& alt_text);
	void closeOpenStructures();

	void addSingleItem( Paragraph::Type type, const std::string& id, const std::string& text, bool joinText);
	void addQuoteItem( Paragraph::Type startType);
	void openAutoCloseItem( Paragraph::Type startType, const char* prefix, int lidx);
	void closeAutoCloseItem( Paragraph::Type startType);
	void closeDanglingStructures( const Paragraph::Type& starttype);
	void checkStartEndSectionBalance( const std::vector<Paragraph>::const_iterator& start, const std::vector<Paragraph>::const_iterator& end);
	bool checkTableDefExists( const char* action);
	void addTableCellIdentifierAttributes( const char* prefix, const std::set<int>& indices);
	std::string passageKey( const std::vector<Paragraph>::const_iterator& begin, const std::vector<Paragraph>::const_iterator& end);
	void processParsedCitation( std::vector<Paragraph>& dest, std::vector<Paragraph>::const_iterator pi, std::vector<Paragraph>::const_iterator pe);

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
	struct CellPosition
	{
		int row;
		int col;

		CellPosition()				:row(0),col(0){}
		CellPosition( int row_, int col_)	:row(row_),col(col_){}
		CellPosition( const CellPosition& o)	:row(o.row),col(o.col){}

		bool operator < (const CellPosition& o) const
		{
			if (row < o.row) return true;
			if (row > o.row) return false;
			return (col < o.col);
		}
	};
	typedef std::map<CellPosition,int> CellPositionStartMap;

	struct TableDef
	{
		int rowiter;
		int coliter;
		int start;
		CellPositionStartMap cellmap;

		explicit TableDef( int start_)
			:rowiter(0),coliter(0),start(start_),cellmap(){}
		TableDef( const TableDef& o)
			:rowiter(o.rowiter),coliter(o.coliter),start(o.start),cellmap(o.cellmap){}

		void defineCell( int startidx, int rowspan, int colspan)
		{
			int ri = rowiter;
			int ci = coliter;

			CellPositionStartMap::const_iterator mi = cellmap.find( CellPosition( ri, ci));
			for (; mi != cellmap.end(); mi = cellmap.find( CellPosition( ri, ++ci))){}
			int rii = 0;
			for (; rii < rowspan; ++rii)
			{
				int cii = 0;
				for (; cii < colspan; ++cii)
				{
					cellmap[ CellPosition( ri+rii, ci+cii)] = startidx;
				}
			}
		}

		void nextRow()
		{
			++rowiter;
			coliter = 0;
		}
		void nextCol( int colspan)
		{
			coliter += colspan;
		}
	};

private:
	std::string m_fileId;
	std::vector<Paragraph> m_parar;
	std::vector<Paragraph> m_citations;
	std::vector<Paragraph> m_tables;
	std::vector<Paragraph> m_refs;
	std::map<std::string,std::string> m_citationmap;
	std::map<std::string,std::string> m_refmap;
	std::vector<StructRef> m_structStack;
	std::vector<TableDef> m_tableDefs;
	std::vector<std::string> m_errors;
	std::set<std::string> m_unresolved;
	int m_nofErrors;
	int m_tableCnt;
	int m_citationCnt;
	int m_refCnt;
	int m_lastHeadingIdx;
};


}//namespace
#endif

