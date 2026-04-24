add_library(2plogger_dxcc STATIC DxccDatabase.cpp)
target_include_directories(2plogger_dxcc PUBLIC ${CMAKE_SOURCE_DIR}/src)
# Pure C++20 — no Qt dependency
