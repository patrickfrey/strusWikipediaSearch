cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
include_directories(
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
add_library( modstrus_storage_wikipedia_search  MODULE  modstrus_storage_wikipedia_search.cpp )
set_target_properties( modstrus_storage_wikipedia_search PROPERTIES PREFIX "")
target_link_libraries( modstrus_storage_wikipedia_search strus_module )


# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS modstrus_storage_wikipedia_search
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus/modules
	   RUNTIME DESTINATION bin )



