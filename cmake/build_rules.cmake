# -----------------------------------------------------------------------------------------------
# Defines the flags for compiler and linker and some build environment settings
# -----------------------------------------------------------------------------------------------
# The following definition requires CMake >= 3.1
set( CMAKE_CXX_COMPILE_FEATURES 
	"cxx_long_long_type"     # because of boost using 'long long' 
)
# Temporary hack to build without warnings with CMake < 3.1:
IF (CPP_LANGUAGE_VERSION STREQUAL "0x")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
ELSEIF (CPP_LANGUAGE_VERSION STREQUAL "11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
ELSEIF (CPP_LANGUAGE_VERSION STREQUAL "14")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
ELSEIF (CPP_LANGUAGE_VERSION STREQUAL "17")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
ELSEIF (CPP_LANGUAGE_VERSION STREQUAL "98")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98")
ELSE (CPP_LANGUAGE_VERSION STREQUAL "0x")
if (HAVE_CXX11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif (HAVE_CXX11)
ENDIF (CPP_LANGUAGE_VERSION STREQUAL "0x")

IF( CMAKE_CXX_FLAGS MATCHES "[-]std=c[+][+](11|14|17)" )
set( STRUS_CXX_STD_11 TRUE )
ENDIF( CMAKE_CXX_FLAGS MATCHES "[-]std=c[+][+](11|14|17)" )

IF (C_LANGUAGE_VERSION STREQUAL "99")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
ELSEIF (C_LANGUAGE_VERSION STREQUAL "90")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c90")
ELSEIF (C_LANGUAGE_VERSION STREQUAL "17")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17")
ELSE (C_LANGUAGE_VERSION STREQUAL "99")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
ENDIF (C_LANGUAGE_VERSION STREQUAL "99")

MESSAGE( STATUS "Debug postfix: '${CMAKE_DEBUG_POSTFIX}'" )
IF(CMAKE_BUILD_TYPE MATCHES RELEASE)
    set( VISIBILITY_FLAGS "-fvisibility=hidden" )
ELSE(CMAKE_BUILD_TYPE MATCHES RELEASE)
IF("${CMAKE_CXX_COMPILER_ID}" MATCHES "[cC]lang")
    set( VISIBILITY_FLAGS "-fstandalone-debug" )
ELSE()
    set( VISIBILITY_FLAGS "" )
ENDIF()
ENDIF(CMAKE_BUILD_TYPE MATCHES RELEASE)

set_property(GLOBAL PROPERTY rule_launch_compile ccache)
set_property(GLOBAL PROPERTY rule_launch_link ccache)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -pedantic -Wfatal-errors ${VISIBILITY_FLAGS}" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wall -pedantic -Wfatal-errors" )
endif()
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "[cC]lang")
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -pedantic -Wfatal-errors ${VISIBILITY_FLAGS} -Wno-unused-private-field" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wall -pedantic -Wfatal-errors" )
endif()

if(WIN32)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_WIN32_WINNT=0x0504")
else()
set( CMAKE_THREAD_PREFER_PTHREAD TRUE )
find_package( Threads REQUIRED )
if( NOT APPLE )
if(CMAKE_USE_PTHREADS_INIT)
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread" )
set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread" )
endif()
endif( NOT APPLE )
endif()

foreach(flag ${CXX11_FEATURE_LIST})
   set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D${flag} -pthread" )
endforeach()

