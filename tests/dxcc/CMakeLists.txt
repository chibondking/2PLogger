add_executable(2plogger_dxcc_tests test_dxcc.cpp)
target_include_directories(2plogger_dxcc_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(2plogger_dxcc_tests PRIVATE
    2plogger_dxcc
    Catch2::Catch2WithMain
)

# Pass the absolute path to cty.dat so tests work from any working directory.
target_compile_definitions(2plogger_dxcc_tests PRIVATE
    CTY_DAT_PATH="${CMAKE_SOURCE_DIR}/data/cty.dat"
)

include(Catch)
catch_discover_tests(2plogger_dxcc_tests)
