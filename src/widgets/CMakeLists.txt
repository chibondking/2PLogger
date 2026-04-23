add_library(2plogger_widgets STATIC EntryWidget.cpp)
target_include_directories(2plogger_widgets PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(2plogger_widgets PUBLIC Qt6::Core Qt6::Widgets)
