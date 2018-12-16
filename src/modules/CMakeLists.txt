cmake_minimum_required(VERSION 2.8 FATAL_ERROR )

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
include_directories(
	"${strusmodule_INCLUDE_DIRS}"
	"${strusbase_INCLUDE_DIRS}"
	"${strus_INCLUDE_DIRS}"
	"${strusanalyzer_INCLUDE_DIRS}"
	"${Boost_INCLUDE_DIRS}"
	"${Intl_INCLUDE_DIRS}"
	${CMAKE_CURRENT_BINARY_DIR}
	${MAIN_SOURCE_DIR}
)

link_directories(
	"${strusbase_LIBRARY_DIRS}"
	"${strus_LIBRARY_DIRS}"
	"${strusanalyzer_LIBRARY_DIRS}"
	"${strusmodule_LIBRARY_DIRS}"
	"${Boost_LIBRARY_DIRS}"
)

# -------------------------------------------
# LIBRARY
# -------------------------------------------
add_cppcheck( strus_module libstrus_module_normalizer_entityid.cpp )

add_library( libstrus_module_normalizer_entityid MODULE libstrus_module_normalizer_entityid.cpp )
target_link_libraries( libstrus_module_normalizer_entityid ${CMAKE_DL_LIBS} strus_base strus_analyzer strus_textproc strus_module ${Intl_LIBRARIES} )

set_target_properties(
    libstrus_module_normalizer_entityid
    PROPERTIES PREFIX "" DEBUG_POSTFIX "${CMAKE_DEBUG_POSTFIX}"
)
# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS libstrus_module_normalizer_entityid
           LIBRARY DESTINATION ${LIB_INSTALL_DIR}/strus/modules
	   RUNTIME DESTINATION bin )


