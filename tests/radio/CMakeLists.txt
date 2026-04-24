add_executable(2plogger_radio_tests test_radio_mode.cpp)
target_include_directories(2plogger_radio_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(2plogger_radio_tests PRIVATE
    2plogger_radio
    Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(2plogger_radio_tests)
