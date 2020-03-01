cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set( TESTBIN  ${CMAKE_BINARY_DIR}/src/wikimediaToXml/strusWikimediaToXml )
set( TESTDIR  ${PROJECT_SOURCE_DIR}/tests/wikimediaToXml4 )
add_test( WikimediaToXml4 ${TESTBIN}  -B -n 0 -P 10000 --test ${TESTDIR}/EXP  ${TESTDIR}/1844_United_States_presidential_election.txt )
