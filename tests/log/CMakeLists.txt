# Separate executable so we can provide our own main() with QCoreApplication.
add_executable(2plogger_log_tests
    main.cpp
    test_database.cpp
)
target_include_directories(2plogger_log_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(2plogger_log_tests PRIVATE
    2plogger_log
    Qt6::Core
    Qt6::Sql
    Catch2::Catch2      # not Catch2WithMain — we supply main()
)
include(Catch)
catch_discover_tests(2plogger_log_tests)
