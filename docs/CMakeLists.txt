find_package(Doxygen REQUIRED)

get_target_property(pmql_header_dir pmql INTERFACE_INCLUDE_DIRECTORIES)
file(GLOB_RECURSE pmql_headers ${pmql_header_dir}/*.h)

set(doxygen_input_dir  ${PROJECT_SOURCE_DIR}/pmql)
set(doxygen_output_dir ${CMAKE_CURRENT_BINARY_DIR}/docs/doxygen)
set(doxygen_index_file ${doxygen_output_dir}/html/index.html)
set(doxyfile_in        ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(doxyfile_out       ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

configure_file(${doxyfile_in} ${doxyfile_out} @ONLY)

file(MAKE_DIRECTORY ${doxygen_output_dir})

add_custom_command(
    OUTPUT ${doxygen_index_file}
    DEPENDS ${pmql_headers}
    COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile_out}
    MAIN_DEPENDENCY ${doxyfile_out} ${doxyfile_in}
    COMMENT "Generating docs")

add_custom_target(Doxygen ALL DEPENDS ${doxygen_index_file})
