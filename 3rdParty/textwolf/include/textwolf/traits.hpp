/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef __TEXTWOLF_TRAITS_HPP__
#define __TEXTWOLF_TRAITS_HPP__
/// \file textwolf/traits.hpp
/// \brief Type traits

namespace textwolf {
namespace traits {

/// \class TypeCheck
/// \brief Test structure to stear the compiler
class TypeCheck
{
public:
	struct YES {};
	struct NO {};

	template<typename T, typename U>
	struct is_same 
	{
		static const NO type() {return NO();}
	};
	
	template<typename T>
	struct is_same<T,T>
	{
		static const YES type() {return YES();} 
	};
};

}}//namespace
#endif
