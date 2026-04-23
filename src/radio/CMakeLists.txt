add_library(2plogger_radio STATIC RadioInterface.cpp)
target_include_directories(2plogger_radio PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(2plogger_radio PUBLIC Qt6::Core Qt6::Network)
