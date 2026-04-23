add_library(2plogger_export STATIC AdifExporter.cpp)
target_include_directories(2plogger_export PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(2plogger_export PUBLIC Qt6::Core)
