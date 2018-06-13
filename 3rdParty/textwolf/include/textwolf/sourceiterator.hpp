/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \file textwolf/sourceiterator.hpp
/// \brief textwolf byte source iterator template

#ifndef __TEXTWOLF_SOURCE_ITERATOR_HPP__
#define __TEXTWOLF_SOURCE_ITERATOR_HPP__
#include "textwolf/exception.hpp"
#include "textwolf/position.hpp"
#include <cstdlib>
#include <stdexcept>
#include <setjmp.h>

/// \namespace textwolf
/// \brief Toplevel namespace of the library
namespace textwolf {

/// \class SrcIterator
/// \brief Input iterator as source for the XML scanner with the possibility of being fed chunk by chunk
class SrcIterator
	:public throws_exception 
{
public:
	/// \brief Empty constructor
	SrcIterator()
		:m_start(0)
		,m_itr(0)
		,m_end(0)
		,m_eom(0)
		,m_abspos(0){}

	/// \brief Copy constructor
	/// \param [in] o iterator to copy
	SrcIterator( const SrcIterator& o)
		:m_start(o.m_start)
		,m_itr(o.m_itr)
		,m_end(o.m_end)
		,m_eom(o.m_eom)
		,m_abspos(o.m_abspos){}

	/// \brief Constructor
	/// \param [in] buf source chunk to iterate on
	/// \param [in] size size of source chunk to iterate on in bytes
	/// \param [in] eom_ trigger to activate if end of data has been reached (no next chunk anymore)
	SrcIterator( const char* buf, std::size_t size, jmp_buf* eom_=0)
		:m_start(const_cast<char*>(buf))
		,m_itr(const_cast<char*>(buf))
		,m_end(m_itr+size)
		,m_eom(eom_)
		,m_abspos(size){}

	/// \brief Assingment operator
	SrcIterator& operator=( const SrcIterator& o)
	{
		m_start = o.m_start;
		m_itr = o.m_itr;
		m_end = o.m_end;
		m_eom = o.m_eom;
		m_abspos = o.m_abspos;
		return *this;
	}

	/// \brief Element access operator (required by textwolf for an input iterator)
	inline char operator*()
	{
		if (m_itr >= m_end)
		{
			if (m_eom) longjmp(*m_eom,1);
			return 0;
		}
		return *m_itr;
	}

	/// \brief Prefix increment operator (required by textwolf for an input iterator)
	inline SrcIterator& operator++()
	{
		++m_itr;
		return *this;
	}

	/// \brief Get the iterator difference in bytes
	inline std::size_t operator-( const SrcIterator& b) const
	{
		if (b.m_end != m_end || m_itr < b.m_itr) throw exception( IllegalParam);
		return m_itr - b.m_itr;
	}

	/// \brief Feed input to the source iterator
	/// \param[in] buf poiner to start of input
	/// \param[in] size size of input passed in bytes
	/// \param[in] eom longjmp to call with parameter 1, if the end of data has been reached before EOF (null termination), eom=null, if the chunk passed contains the complete reset of the input and eof (null) can be returned if we reach the end
	void putInput( const char* buf, std::size_t size, jmp_buf* eom=0)
	{
		m_abspos += size;
		m_start = m_itr = const_cast<char*>(buf);
		m_end = m_itr+size;
		m_eom = eom;
	}

	/// \brief Get the current position in the current chunk parsed
	/// \remark Does not return the absolute position in the source parsed but the position in the chunk
	std::size_t getPosition() const
	{
		return (m_end >= m_itr)?(m_itr-m_start):0;
	}

	PositionIndex position() const
	{
		return m_abspos - (m_end - m_itr);
	}

	bool endOfChunk() const
	{
		return (m_itr >= m_end);
	}

private:
	char* m_start;
	char* m_itr;
	char* m_end;
	jmp_buf* m_eom;
	PositionIndex m_abspos;
};

}//namespace
#endif


