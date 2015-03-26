cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set( source_files
	inputStream.cpp
)
include_directories(  
	"${PROJECT_SOURCE_DIR}/3rdParty/textwolf/include"
)
link_directories(
)


# ------------------------------
# PROGRAMS
# ------------------------------
add_executable( strusWikimediaToXml strusWikimediaToXml.cpp ${source_files} )
target_link_libraries( strusWikimediaToXml  )
add_executable( validateXml validateXml.cpp ${source_files} )

# ------------------------------
# INSTALLATION
# ------------------------------
install( TARGETS strusWikimediaToXml
	   RUNTIME DESTINATION bin )

