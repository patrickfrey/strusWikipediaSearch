cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set( TESTBIN  ${CMAKE_BINARY_DIR}/src/wikimediaToXml/strusWikimediaToXml )
set( TESTDIR  ${PROJECT_SOURCE_DIR}/tests/wikimediaToXml2 )
add_test( WikimediaToXml2_valid ${TESTBIN}  -B -n 0 -P 10000 --test ${TESTDIR}/EXP  ${TESTDIR}/United_States_presidential_election_1852.txt )
add_test( WikimediaToXml2_strus ${TESTBIN}  -I -B -n 0 -P 10000 --test ${TESTDIR}/EXP_I ${TESTDIR}/United_States_presidential_election_1852.txt )
