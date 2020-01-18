
add_definitions("-DTESTING")

include_directories(".." ${GLIB_INCLUDE_DIRS} ${CHECK_INCLUDE_DIRS})

set(basic2_SRCS)
foreach(SRC ${basic_SRCS})
    set(basic2_SRCS ${basic2_SRCS} ../${SRC})
endforeach(SRC ${basic_SRCS})

add_executable(unittest
    test.c
    test_globals.c
    test_activeconnection.c
    test_connection.c
    test_network.c
    test_port.c
    test_dbus.c
    test_setting.c
    mock_nm.c
    ${basic2_SRCS}
)

target_link_libraries(unittest ${CHECK_LIBRARIES} pthread ${GLIB_LIBRARIES} ${DBUSGLIB_LIBRARIES} ${UUID_LIBRARIES} ${OPENLMI_LIBRARIES})

add_custom_target(unittests
    ${CMAKE_CURRENT_BINARY_DIR}/unittest
)
