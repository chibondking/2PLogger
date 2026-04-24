find_package(Catch2 3 QUIET)
if(NOT Catch2_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.3
    )
    FetchContent_MakeAvailable(Catch2)
endif()

# Placeholder suite (no Qt dependency, uses Catch2WithMain)
add_executable(2plogger_tests test_placeholder.cpp)
target_include_directories(2plogger_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(2plogger_tests PRIVATE
    2plogger_log
    2plogger_dxcc
    2plogger_contest
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(2plogger_tests)

# Database / log layer tests (needs QCoreApplication — provides own main)
add_subdirectory(log)

# DXCC lookup tests (pure C++20, no Qt)
add_subdirectory(dxcc)

# Radio mode/profile tests (pure C++20, no Qt)
add_subdirectory(radio)
