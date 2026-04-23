add_library(2plogger_dxcc STATIC DxccDatabase.cpp)
target_include_directories(2plogger_dxcc PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(2plogger_dxcc PUBLIC Qt6::Core)
