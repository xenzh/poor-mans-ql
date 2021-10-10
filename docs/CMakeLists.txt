find_package(Doxygen REQUIRED)

get_target_property(pmql_header_dir pmql INTERFACE_INCLUDE_DIRECTORIES)
file(GLOB_RECURSE pmql_headers ${pmql_header_dir}/*.h)

set(doxygen_input_dir  ${PROJECT_SOURCE_DIR}/pmql)
set(doxygen_output_dir ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(doxygen_index_file ${doxygen_output_dir}/xml/index.xml)
set(doxyfile_in        ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(doxyfile_out       ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

configure_file(${doxyfile_in} ${doxyfile_out} @ONLY)

file(MAKE_DIRECTORY ${doxygen_output_dir})

add_custom_command(
    OUTPUT ${doxygen_index_file}
    DEPENDS ${pmql_headers}
    COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile_out}
    MAIN_DEPENDENCY ${doxyfile_out} ${doxyfile_in}
    COMMENT "Generating docs"
    VERBATIM
)

add_custom_target(doxygen ALL DEPENDS ${doxygen_index_file})


find_package(Sphinx REQUIRED)

set(sphinx_source     ${CMAKE_CURRENT_SOURCE_DIR})
set(sphinx_build      ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
set(sphinx_index_file ${sphinx_build}/index.html)

add_custom_command(
    OUTPUT ${sphinx_index_file}
    COMMAND
        ${SPHINX_EXECUTABLE} -b html
        -Dbreathe_projects.pmql=${doxygen_output_dir}/xml
        ${sphinx_source}
        ${sphinx_build}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/index.rst
        ${doxygen_index_file}
    MAIN_DEPENDENCY ${sphinx_source}/conf.py
    COMMENT "Generating documentation with Sphinx"
)

add_custom_target(sphinx ALL DEPENDS ${sphinx_index_file})
