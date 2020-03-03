cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set( TESTBIN  ${CMAKE_BINARY_DIR}/src/wikimediaToXml/strusWikimediaToXml )
set( TESTDIR  ${PROJECT_SOURCE_DIR}/tests/wikimediaToXml5 )
add_test( WikimediaToXml5 ${TESTBIN}  -B -n 0 -P 10000 --test ${TESTDIR}/EXP  ${TESTDIR}/Faraday_cage.txt )
