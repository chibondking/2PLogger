add_library(2plogger_app STATIC MainWindow.cpp)
target_include_directories(2plogger_app PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(2plogger_app PUBLIC Qt6::Widgets)
