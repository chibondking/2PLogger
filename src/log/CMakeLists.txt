add_library(2plogger_log STATIC LogBook.cpp)
target_include_directories(2plogger_log PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(2plogger_log PUBLIC Qt6::Core Qt6::Sql)
