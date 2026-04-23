add_library(2plogger_cw STATIC CwEngine.cpp)
target_include_directories(2plogger_cw PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(2plogger_cw PUBLIC Qt6::Core)
