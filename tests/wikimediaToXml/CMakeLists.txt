cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set( TESTBIN ${CMAKE_BINARY_DIR}/src/wikimediaToXml/strusWikimediaToXml )
add_test( WikimediaToXml_valid ${TESTBIN}  -B -n 0 -P 10000 --test ${PROJECT_SOURCE_DIR}/tests/wikimediaToXml/EXP ${PROJECT_SOURCE_DIR}/tests/wikimediaToXml/input.xml )
add_test( WikimediaToXml_strus ${TESTBIN}  -I -B -n 0 -P 10000 --test ${PROJECT_SOURCE_DIR}/tests/wikimediaToXml/EXP_I ${PROJECT_SOURCE_DIR}/tests/wikimediaToXml/input.xml )
