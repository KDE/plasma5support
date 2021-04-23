add_subdirectory(plasma5support)
#add_subdirectory(declarativeimports)


ecm_qt_install_logging_categories(
    EXPORT PLASMA5SUPPORT
    FILE plasma5support.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)
