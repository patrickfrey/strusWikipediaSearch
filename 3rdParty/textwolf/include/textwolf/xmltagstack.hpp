/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \file textwolf/xmltagstack.hpp
/// \brief textwolf XML printer tag stack

#ifndef __TEXTWOLF_XML_TAG_STACK_HPP__
#define __TEXTWOLF_XML_TAG_STACK_HPP__
#include "textwolf/exception.hpp"
#include <cstring>
#include <cstdlib>

/// \namespace textwolf
/// \brief Toplevel namespace of the library
namespace textwolf {

/// \class TagStack
/// \brief stack of tag names
class TagStack
	:public throws_exception
{
public:
	/// \brief Destructor
	~TagStack()
	{
		if (m_ptr) std::free( m_ptr);
	}

	/// \brief Default constructor
	TagStack()
		:m_ptr(0),m_pos(0),m_size(InitSize)
	{
		if ((m_ptr=(char*)std::malloc( m_size)) == 0) throw std::bad_alloc();
	}
	/// \brief Copy constructor
	TagStack( const TagStack& o)
		:m_ptr(0),m_pos(o.m_pos),m_size(o.m_size)
	{
		if ((m_ptr=(char*)std::malloc( m_size)) == 0) throw std::bad_alloc();
		std::memcpy( m_ptr, o.m_ptr, m_pos);
	}

	/// \brief Push a tag on top
	/// \param[out] pp pointer to tag value to push
	/// \param[out] nn size of tag value to push in bytes
	void push( const char* pp, std::size_t nn)
	{
		std::size_t align = getAlign( nn);
		std::size_t ofs = nn + align + sizeof( std::size_t);
		if (m_pos + ofs > m_size)
		{
			while (m_pos + ofs > m_size) m_size *= 2;
			if (m_pos + ofs > m_size) throw std::bad_alloc();
			if (nn > ofs) throw exception( InvalidTagOffset);
			char* xx = (char*)std::realloc( m_ptr, m_size);
			if (!xx) throw std::bad_alloc();
			m_ptr = xx;
		}
		std::memcpy( m_ptr + m_pos, pp, nn);
		m_pos += ofs;
		void* tt = m_ptr + m_pos - sizeof( std::size_t);
		*(std::size_t*)(tt) = nn;
	}

	/// \brief Get the topmost tag
	/// \param[out] element pointer to topmost tag value
	/// \param[out] elementsize size of topmost tag value in bytes
	/// \return true on success, false if the stack is empty
	bool top( const void*& element, std::size_t& elementsize)
	{
		std::size_t ofs = topofs(elementsize);
		if (!ofs) return false;
		element = m_ptr + m_pos - ofs;
		return true;
	}

	/// \brief Pop (remove) the topmost tag
	void pop()
	{
		std::size_t elementsize=0;
		std::size_t ofs = topofs(elementsize);
		if (m_pos < ofs) throw exception( CorruptTagStack);
		m_pos -= ofs;
	}

	/// \brief Find out if the stack is empty
	/// \return true if yes
	bool empty() const
	{
		return (m_pos == 0);
	}

	void clear()
	{
		m_pos = 0;
	}

private:
	std::size_t topofs( std::size_t& elementsize)
	{
		if (m_pos < sizeof( std::size_t)) return 0;
		void* tt = m_ptr + (m_pos - sizeof( std::size_t));
		elementsize = *(std::size_t*)(tt);
		std::size_t align = getAlign( elementsize);
		std::size_t ofs = elementsize + align + sizeof( std::size_t);
		if (ofs > m_pos) return 0;
		return ofs;
	}
private:
	enum {InitSize=256};
	char* m_ptr;		///< pointer to the tag hierarchy stack buffer
	std::size_t m_pos;	///< current position in the tag hierarchy stack buffer
	std::size_t m_size;	///< current size of the tag hierarchy stack buffer

	static std::size_t getAlign( std::size_t n)
	{
		return (sizeof(std::size_t) - (n & (sizeof(std::size_t)-1))) & (sizeof(std::size_t)-1);
	}
};

} //namespace
#endif
