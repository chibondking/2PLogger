add_subdirectory(app)
add_subdirectory(radio)
add_subdirectory(cluster)
add_subdirectory(bandmap)
add_subdirectory(log)
add_subdirectory(contest)
add_subdirectory(dxcc)
add_subdirectory(export)
add_subdirectory(udp)
add_subdirectory(cw)
add_subdirectory(widgets)

add_executable(2plogger main.cpp)
target_include_directories(2plogger PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(2plogger PRIVATE
    2plogger_app
    2plogger_radio
    2plogger_cluster
    2plogger_bandmap
    2plogger_log
    2plogger_contest
    2plogger_dxcc
    2plogger_export
    2plogger_udp
    2plogger_cw
    2plogger_widgets
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
)
