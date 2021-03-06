/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \file textwolf/exception.hpp
/// \brief Definition of exceptions with containing error codes thrown by textwolf

#ifndef __TEXTWOLF_EXCEPTION_HPP__
#define __TEXTWOLF_EXCEPTION_HPP__
#include <exception>
#include <stdexcept>

namespace textwolf {

/// \class throws_exception
/// \brief Base class for structures that can throw exceptions for non recoverable errors
struct throws_exception
{
	/// \enum Cause
	/// \brief Enumeration of error cases
	enum Cause
	{
		Unknown,			///< uknown error
		DimOutOfRange,			///< memory reserved for statically allocated table or memory block is too small. Increase the size of memory block passed to the XML path select automaton. Usage error
		StateNumbersNotAscending,	///< XML scanner automaton definition check failed. Labels of states must be equal to their indices. Internal textwolf error
		InvalidParamState,		///< parameter check (for state) in automaton definition failed. Internal textwolf error
		InvalidParamChar,		///< parameter check (for control character) in automaton definition failed. Internal textwolf error
		DuplicateStateTransition,	///< duplicate transition definition in automaton. Internal textwolf error
		InvalidState,			///< invalid state definition in automaton. Internal textwolf error
		IllegalParam,			///< parameter check in automaton definition failed. Internal textwolf error
		IllegalAttributeName,		///< invalid string for a tag or attribute in the automaton definition. Usage error
		OutOfMem,			///< out of memory in the automaton definition. System error (std::bad_alloc)
		ArrayBoundsReadWrite,		///< invalid array access. Internal textwolf error
		NotAllowedOperation,		///< defining an operation in an automaton definition that is not allowed there. Usage error
		FileReadError,			///< error reading a file. System error
		IllegalXmlHeader,		///< illegal XML header (more than 4 null bytes in a row). Usage error
		InvalidTagOffset,		///< internal error in the tag stack. Internal textwolf error
		CorruptTagStack,		///< currupted tag stack. Internal textwolf error
		CodePageIndexNotSupported	///< the index of the code page specified for a character set encoding is unknown to textwolf. Usage error
	};
};

/// \class exception
/// \brief textwolf exception class
struct exception	:public std::runtime_error
{
	typedef throws_exception::Cause Cause;
	Cause cause;					//< exception cause tag

	/// \brief Constructor
	/// \return exception object
	exception (Cause p_cause) throw()
		:std::runtime_error("textwolf error in XML"), cause(p_cause) {}
	/// \brief Copy constructor
	exception (const exception& orig) throw()
		:std::runtime_error("textwolf error in XML"), cause(orig.cause) {}
	/// \brief Destructor
	virtual ~exception() throw() {}

	/// \brief Assignement
	/// \param[in] orig exception to copy
	/// \return *this
	exception& operator= (const exception& orig) throw()
			{cause=orig.cause; return *this;}

	/// \brief Exception message
	/// \return exception cause as string
	virtual const char* what() const throw()
	{
		// enumeration of exception causes as strings
		static const char* nameCause[ 17] = {
			"Unknown","DimOutOfRange","StateNumbersNotAscending","InvalidParamState",
			"InvalidParamChar","DuplicateStateTransition","InvalidState","IllegalParam",
			"IllegalAttributeName","OutOfMem","ArrayBoundsReadWrite","NotAllowedOperation"
			"FileReadError","IllegalXmlHeader","InvalidTagOffset","CorruptTagStack",
			"CodePageIndexNotSupported"
		};
		return nameCause[ (unsigned int) cause];
	}
};

}//namespace
#endif
