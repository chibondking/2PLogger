add_library(2plogger_bandmap STATIC BandmapModel.cpp)
target_include_directories(2plogger_bandmap PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(2plogger_bandmap PUBLIC Qt6::Core Qt6::Widgets)
