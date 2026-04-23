add_library(2plogger_cluster STATIC ClusterClient.cpp)
target_include_directories(2plogger_cluster PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(2plogger_cluster PUBLIC Qt6::Core Qt6::Network)
