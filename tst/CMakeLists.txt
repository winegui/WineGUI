include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(bottle_config_migration_test
  bottle_config_migration_test.cc
)
target_compile_features(bottle_config_migration_test PUBLIC cxx_std_23)
set_target_properties(bottle_config_migration_test PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(bottle_config_migration_test PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_BINARY_DIR}
)
target_link_libraries(bottle_config_migration_test PRIVATE
  ${PROJECT_TEST_TARGET_LIB}-bottle-config
  gtest_main
)
add_test(NAME bottle_config_migration_test COMMAND bottle_config_migration_test)

add_custom_target(tests
  COMMAND env GTEST_COLOR=1 ${CMAKE_CTEST_COMMAND} --verbose --output-on-failure
  DEPENDS bottle_config_migration_test
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tst
  COMMENT "Execute all unit tests"
  VERBATIM
)
