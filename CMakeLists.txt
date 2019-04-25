cmake_minimum_required(VERSION 2.6 FATAL_ERROR )

# must be done BEFORE project in order to work!
include(cmake/install_prefix.cmake)

# Project globals:
project( StrusWikipediaSearch )

set( CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" )
include( cmake/cxx11.cmake )
check_for_cxx11_compiler( HAVE_CXX11 )
include( cmake/dist.cmake )
include( cmake/LinuxDistribution.cmake )
include( cmake/build_rules.cmake )
include( cmake/boost.cmake)
include( cmake/find_strus_package.cmake )
include( cmake/link_rules.cmake )
include( cmake/intl.cmake )
include( cmake/cppcheck.cmake )

find_strus_package( base )
find_strus_package( core )
find_strus_package( analyzer )
find_strus_package( module )
find_strus_package( rpc )
find_strus_package( bindings )

set( CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" )

if( APPLE OR WIN32 )
	set( PROJECT_BIN_AFFIX "${CMAKE_BUILD_TYPE}/" )
else( APPLE OR WIN32 )
	set( PROJECT_BIN_AFFIX "" )
endif( APPLE OR WIN32 )

# Module path:
if (NOT STRUS_MODULE_PATH)
set( STRUS_MODULE_PATH  "${CMAKE_INSTALL_PREFIX}/${LIB_INSTALL_DIR}/strus/modules" )
endif (NOT STRUS_MODULE_PATH)
MESSAGE( STATUS  "Strus module path: ${STRUS_MODULE_PATH}" )

set( STRUS_GETTEXT_PACKAGE "strus-dom" )	#... 1st parameter of bindtextdomain(...)
set( STRUS_GETTEXT_LOCALEDIR "" )			#... 2nd parameter of bindtextdomain(...)

set( STRUS_SERVER_PORT 7184 )
set( STRUS_SERVER_ID  "isrv1" )
configure_file( "${PROJECT_SOURCE_DIR}/config/config.js.in"  "${CMAKE_CURRENT_BINARY_DIR}/config/config_${STRUS_SERVER_ID}.js"  @ONLY )
set( STRUS_SERVER_PORT 7185 )
set( STRUS_SERVER_ID  "isrv2" )
configure_file( "${PROJECT_SOURCE_DIR}/config/config.js.in"  "${CMAKE_CURRENT_BINARY_DIR}/config/config_${STRUS_SERVER_ID}.js"  @ONLY )
set( STRUS_SERVER_PORT 7186 )
set( STRUS_SERVER_ID  "isrv3" )
configure_file( "${PROJECT_SOURCE_DIR}/config/config.js.in"  "${CMAKE_CURRENT_BINARY_DIR}/config/config_${STRUS_SERVER_ID}.js"  @ONLY )
set( STRUS_SERVER_PORT 7187 )
set( STRUS_SERVER_ID  "isrv4" )
configure_file( "${PROJECT_SOURCE_DIR}/config/config.js.in"  "${CMAKE_CURRENT_BINARY_DIR}/config/config_${STRUS_SERVER_ID}.js"  @ONLY )
set( STRUS_SERVER_PORT 7191 )
set( STRUS_SERVER_ID  "vsrv1" )
configure_file( "${PROJECT_SOURCE_DIR}/config/config.js.in"  "${CMAKE_CURRENT_BINARY_DIR}/config/config_${STRUS_SERVER_ID}.js"  @ONLY )

set( STRUS_STORAGE_ID 1 )
configure_file( "${PROJECT_SOURCE_DIR}/config/istorage.json.in"  "${CMAKE_CURRENT_BINARY_DIR}/config/istorage${STRUS_STORAGE_ID}.json"  @ONLY )
set( STRUS_STORAGE_ID 2 )
configure_file( "${PROJECT_SOURCE_DIR}/config/istorage.json.in"  "${CMAKE_CURRENT_BINARY_DIR}/config/istorage${STRUS_STORAGE_ID}.json"  @ONLY )
set( STRUS_STORAGE_ID 3 )
configure_file( "${PROJECT_SOURCE_DIR}/config/istorage.json.in"  "${CMAKE_CURRENT_BINARY_DIR}/config/istorage${STRUS_STORAGE_ID}.json"  @ONLY )
set( STRUS_STORAGE_ID 4 )
configure_file( "${PROJECT_SOURCE_DIR}/config/istorage.json.in"  "${CMAKE_CURRENT_BINARY_DIR}/config/istorage${STRUS_STORAGE_ID}.json"  @ONLY )

enable_testing()

# Subdirectories:
add_subdirectory( src )
add_subdirectory( tests )
  
include( cmake/report_build_settings.cmake )

# Install header files:
install( DIRECTORY include/strus
          DESTINATION include
          FILES_MATCHING PATTERN "*.hpp")

include( cmake/standard_targets.cmake )

