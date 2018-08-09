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
set( CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON )

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

