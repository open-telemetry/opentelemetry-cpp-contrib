find_package(GTest CONFIG QUIET)
if(NOT GTest_FOUND)
  message(STATUS "GTest not found, fetching...")
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY
    https://github.com/google/googletest.git
    GIT_TAG
    52eb8108c5bdec04579160ae17225d66034bd723 # v1.17.0
  )
  FetchContent_MakeAvailable(googletest)
endif()

if(NOT GTest_VERSION) 
  message(FATAL_ERROR "GTest version not found")
endif()

if(NOT TARGET GTest::gtest)
  message(FATAL_ERROR "GTest::gtest not found")
endif()
  
if(NOT TARGET GTest::gtest_main)
  message(FATAL_ERROR "GTest::gtest_main not found")
endif()
