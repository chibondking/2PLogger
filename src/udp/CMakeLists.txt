add_library(2plogger_udp STATIC UdpBroadcaster.cpp)
target_include_directories(2plogger_udp PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(2plogger_udp PUBLIC Qt6::Core Qt6::Network)
