/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \file textwolf/position.hpp
/// \brief Definition of position number in source
#ifndef __TEXTWOLF_POSITION_HPP__
#define __TEXTWOLF_POSITION_HPP__

#ifdef BOOST_VERSION
#include <boost/cstdint.hpp>
namespace textwolf {
	/// \typedef Position
	/// \brief Source position index type
	typedef boost::uint64_t PositionIndex;
}//namespace
#else
#ifdef _MSC_VER
#pragma warning(disable:4290)
#include <BaseTsd.h>
namespace textwolf {
	/// \typedef Position
	/// \brief Source position index type
	typedef DWORD64 PositionIndex;
}//namespace
#else
#include <stdint.h>
namespace textwolf {
	/// \typedef Position
	/// \brief Source position index type
	typedef uint64_t PositionIndex;
}//namespace
#endif
#endif

#endif

