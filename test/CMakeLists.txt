# automatically enable catch2 to generate ctest targets
if(CONAN_CATCH2_ROOT_DEBUG)
  include(${CONAN_CATCH2_ROOT_DEBUG}/lib/cmake/Catch2/Catch.cmake)
  #set(CONAN_CATCH2_ROOT_DEBUG ${CONAN_CATCH2_ROOT_DEBUG} PARENT_SCOPE)
else()
  include(${CONAN_CATCH2_ROOT}/lib/cmake/Catch2/Catch.cmake)
  #set(CONAN_CATCH2_ROOT ${CONAN_CATCH2_ROOT} PARENT_SCOPE)
endif()


add_library(catch_main STATIC catch_main.cpp)
target_link_libraries(catch_main PUBLIC CONAN_PKG::catch2)

add_executable(tests_core tests.cpp)
target_link_libraries(tests_core PRIVATE vkfw_warnings vkfw_options catch_main)


# automatically discover tests that are defined in catch based test files you
# can modify the unittests. TEST_PREFIX to whatever you want, or use different
# for different binaries
catch_discover_tests(
  tests_core
  TEST_PREFIX
  "unittests."
  EXTRA_ARGS
  -s
  --reporter=xml
  --out=tests_core.xml)

# Add a file containing a set of constexpr tests
add_executable(constexpr_tests_core constexpr_tests.cpp)
target_link_libraries(constexpr_tests_core vkfw_warnings vkfw_options catch_main)

catch_discover_tests(
  constexpr_tests_core
  TEST_PREFIX
  "constexpr."
  EXTRA_ARGS
  -s
  --reporter=xml
  --out=constexpr_core.xml)

# Disable the constexpr portion of the test, and build again this allows us to
# have an executable that we can debug when things go wrong with the constexpr
# testing
add_executable(relaxed_constexpr_tests_core constexpr_tests.cpp)
target_link_libraries(relaxed_constexpr_tests_core PRIVATE vkfw_warnings vkfw_options catch_main)
target_compile_definitions(relaxed_constexpr_tests_core PRIVATE -DCATCH_CONFIG_RUNTIME_STATIC_REQUIRE)

catch_discover_tests(
  relaxed_constexpr_tests_core
  TEST_PREFIX
  "relaxed_constexpr."
  EXTRA_ARGS
  -s
  --reporter=xml
  --out=relaxed_constexpr_core.xml)
