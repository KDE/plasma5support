include_directories( ${CMAKE_CURRENT_BINARY_DIR}/../../)

set(mouse_engine_SRCS
    mouseengine.cpp
    mouseengine.h
)

if (X11_FOUND)
    set(mouse_engine_SRCS ${mouse_engine_SRCS} cursornotificationhandler.cpp)
endif ()

kcoreaddons_add_plugin(plasma_engine_mouse SOURCES ${mouse_engine_SRCS} INSTALL_NAMESPACE plasma5support/dataengine)
target_link_libraries(plasma_engine_mouse
    Qt::Widgets
    Plasma::Plasma5Support
    KF6::WindowSystem
)

if (X11_FOUND)
    target_link_libraries(plasma_engine_mouse Qt::GuiPrivate)

    target_link_libraries(plasma_engine_mouse X11::X11 X11::Xfixes)
endif ()
