add_library(2plogger_contest STATIC ContestRules.cpp)
target_include_directories(2plogger_contest PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(2plogger_contest PUBLIC Qt6::Core)
