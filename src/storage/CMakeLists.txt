cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
include_directories(
	"${PROJECT_SOURCE_DIR}/include"
	"${Boost_INCLUDE_DIRS}"
	"${strus_INCLUDE_DIRS}"
	"${strusanalyzer_INCLUDE_DIRS}"
	"${strusmodule_INCLUDE_DIRS}"
)

link_directories( 
        "${Boost_LIBRARY_DIRS}"
	"${strus_LIBRARY_DIRS}"
	"${strusanalyzer_LIBRARY_DIRS}"
	"${strusmodule_LIBRARY_DIRS}" )

# ------------------------------------
# LIBRARY
# ------------------------------------
add_executable( strusWikipediaDemoPatchIndexTitle strusPatchIndexTitle.cpp )
target_link_libraries( strusWikipediaDemoPatchIndexTitle  "${Boost_LIBRARIES}" strus_database_leveldb strus_base strus_storage strus_storage_objbuild strus_error strus_module ${Intl_LIBRARIES} )


# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strusWikipediaDemoPatchIndexTitle
           RUNTIME DESTINATION bin )


