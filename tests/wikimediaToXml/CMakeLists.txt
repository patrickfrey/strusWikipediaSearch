cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set( TESTBIN ${CMAKE_BINARY_DIR}/src/wikimediaToXml/strusWikimediaToXml )
set( TESTDIR  ${PROJECT_SOURCE_DIR}/tests/wikimediaToXml )

add_test( WikimediaToXml_valid ${TESTBIN}  -B -n 0 -P 10000 --test ${TESTDIR}/EXP ${TESTDIR}/input.xml )
add_test( WikimediaToXml_strus ${TESTBIN}  -I -B -n 0 -P 10000 --test ${TESTDIR}/EXP_I ${TESTDIR}/input.xml )
