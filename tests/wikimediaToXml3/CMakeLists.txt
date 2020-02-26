cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set( TESTBIN  ${CMAKE_BINARY_DIR}/src/wikimediaToXml/strusWikimediaToXml )
set( TESTDIR  ${PROJECT_SOURCE_DIR}/tests/wikimediaToXml3 )
add_test( WikimediaToXml3 ${TESTBIN}  -B -n 0 -P 10000 --test ${TESTDIR}/EXP  ${TESTDIR}/David_Bowie.txt )
