/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \file textwolf/charset_isolatin.hpp
/// \brief Definition of IsoLatin encodings

#ifndef __TEXTWOLF_CHARSET_ISOLATIN_HPP__
#define __TEXTWOLF_CHARSET_ISOLATIN_HPP__
#include "textwolf/char.hpp"
#include "textwolf/charset_interface.hpp"
#include "textwolf/exception.hpp"
#include "textwolf/codepages.hpp"
#include <cstddef>

namespace textwolf {
namespace charset {

/// \class IsoLatin
/// \brief Character set IsoLatin-1,..IsoLatin-9 (ISO-8859-1,...ISO-8859-9)
struct IsoLatin :public IsoLatinCodePage
{
	enum {MaxChar=0xFFU};

	IsoLatin( const IsoLatin& o)
		:IsoLatinCodePage(o){}
	IsoLatin( unsigned int codePageIdx=1)
		:IsoLatinCodePage(codePageIdx){}

	/// \brief See template<class Iterator>Interface::skip(char*,unsigned int&,Iterator&)
	template <class Iterator>
	static inline void skip( char*, unsigned int& bufpos, Iterator& itr)
	{
		if (bufpos==0)
		{
			++itr;
			++bufpos;
		}
	}

	/// \brief See template<class Iterator>Interface::fetchbytes(char*,unsigned int&,Iterator&)
	template <class Iterator>
	static inline void fetchbytes( char* buf, unsigned int& bufpos, Iterator& itr)
	{
		if (bufpos==0)
		{
			buf[0] = *itr;
			++itr;
			++bufpos;
		}
	}

	/// \brief See template<class Iterator>Interface::asciichar(char*,unsigned int&,Iterator&)
	template <class Iterator>
	static inline signed char asciichar( char* buf, unsigned int& bufpos, Iterator& itr)
	{
		fetchbytes( buf, bufpos, itr);
		return ((unsigned char)(buf[0])>127)?-1:buf[0];
	}

	/// \brief See template<class Iterator>Interface::value(char*,unsigned int&,Iterator&)
	template <class Iterator>
	inline UChar value( char* buf, unsigned int& bufpos, Iterator& itr) const
	{
		fetchbytes( buf, bufpos, itr);
		return ucharcode( buf[0]);
	}

	/// \brief See template<class Buffer>Interface::print(UChar,Buffer&)
	template <class Buffer_>
	void print( UChar chr, Buffer_& buf) const
	{
		char chr_ = invcode( chr);
		if (chr_ == 0)
		{
			char tb[ 32];
			char* cc = tb;
			Encoder::encode( chr, tb, sizeof(tb));
			while (*cc) buf.push_back( *cc++);
		}
		else
		{
			buf.push_back( chr_);
		}
	}

	/// \brief See template<class Buffer>Interface::is_equal( const Interface&, const Interface&)
	static inline bool is_equal( const IsoLatin& a, const IsoLatin& b)
	{
		return IsoLatinCodePage::is_equal( a, b);
	}
};

}//namespace
}//namespace
#endif
