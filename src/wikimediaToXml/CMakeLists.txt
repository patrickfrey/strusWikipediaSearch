cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	outputString.cpp
	documentStructure.cpp
	wikimediaLexer.cpp
	strusWikimediaToXml.cpp
)
include_directories(  
	"${PROJECT_SOURCE_DIR}/3rdParty/textwolf/include"
	"${PROJECT_SOURCE_DIR}/include"
	"${Intl_INCLUDE_DIRS}"
	${Boost_INCLUDE_DIRS}
	"${strusbase_INCLUDE_DIRS}"
)
link_directories(
	${Boost_LIBRARY_DIRS}
	"${strusbase_LIBRARY_DIRS}"
)


# ------------------------------
# PROGRAMS
# ------------------------------
add_executable( strusWikimediaToXml ${source_files} )
target_link_libraries( strusWikimediaToXml  strus_base ${Boost_LIBRARIES} ${Intl_LIBRARIES} )
add_executable( validateXml validateXml.cpp outputString.cpp )
target_link_libraries( validateXml strus_base ${Boost_LIBRARIES} ${Intl_LIBRARIES} )

# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strusWikimediaToXml
	   RUNTIME DESTINATION bin )

