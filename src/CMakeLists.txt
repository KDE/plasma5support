configure_file(config-X11.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-X11.h)

add_subdirectory(plasma5support)
add_subdirectory(declarativeimports)
add_subdirectory(dataengines)

ecm_qt_install_logging_categories(
    EXPORT PLASMA5SUPPORT
    FILE plasma5support.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)
