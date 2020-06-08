find_package(Doxygen REQUIRED)
set(DOXYGEN_CONFIG ${CMAKE_CURRENT_BINARY_DIR}/../doxygen.conf)

# note the option ALL which allows to build the docs together with the application
add_custom_target(Doxygen ALL
  COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CONFIG}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Generating API documentation with Doxygen"
  VERBATIM)
