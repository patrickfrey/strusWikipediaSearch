cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_test( WikimediaToXml_valid strusWikimediaToXml  -B -n 0 -P 10000 --test ${PROJECT_SOURCE_DIR}/tests/wikimediaToXML/EXP ${PROJECT_SOURCE_DIR}/tests/wikimediaToXML/input.xml )
add_test( WikimediaToXml_strus strusWikimediaToXml  -I -B -n 0 -P 10000 --test ${PROJECT_SOURCE_DIR}/tests/wikimediaToXML/EXP_I ${PROJECT_SOURCE_DIR}/tests/wikimediaToXML/input.xml )
