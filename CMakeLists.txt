# minimum requirement
cmake_minimum_required(VERSION 3.5)

# solution
project(normalDistribution)

set(TARGET_NAME         drawPPTX)
set(generateTarget      generatePPTX)
set(EXTRACTED_TEMPLATE  ${CMAKE_BINARY_DIR}/template)
set(TEMPLATE_SOURCE     ${CMAKE_SOURCE_DIR}/templates/)
set(OUTPUT_XML_FILENAME "slide1.xml")
set(CM                  ${CMAKE_COMMAND})
set(GENERATED_FILE      "generated")
set(CMAKE_CXX_STANDARD  11)

# build TARGET_NAME from source code
add_executable(${TARGET_NAME} main.cpp)

macro(ADD_POST_BUILD _TARGET _COMMAND)
    add_custom_command(TARGET ${_TARGET} POST_BUILD COMMAND ${_COMMAND} ${ARGN})
endmacro()

add_custom_target(${generateTarget})

ADD_POST_BUILD(${generateTarget} ${TARGET_NAME})
ADD_POST_BUILD(${generateTarget} ${CM} -E remove_directory ${EXTRACTED_TEMPLATE})
ADD_POST_BUILD(${generateTarget} ${CM} -E make_directory ${EXTRACTED_TEMPLATE})
ADD_POST_BUILD(${generateTarget} ${CM} -E copy_directory ${TEMPLATE_SOURCE} ${EXTRACTED_TEMPLATE})
ADD_POST_BUILD(${generateTarget} ${CM} -E copy ${OUTPUT_XML_FILENAME} ${EXTRACTED_TEMPLATE}/ppt/slides/)
ADD_POST_BUILD(${generateTarget} ${CM} -E remove -f ${CMAKE_BINARY_DIR}/${GENERATED_FILE}.zip)
ADD_POST_BUILD(${generateTarget} ${CM} -E chdir ${EXTRACTED_TEMPLATE} ${CM} -E tar cf ${CMAKE_BINARY_DIR}/${GENERATED_FILE}.zip --format=zip [Content_Types].xml _rels docProps ppt)
ADD_POST_BUILD(${generateTarget} ${CM} -E rename ${CMAKE_BINARY_DIR}/${GENERATED_FILE}.zip ${CMAKE_BINARY_DIR}/${GENERATED_FILE}.pptx)
