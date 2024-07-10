
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-plasma5support.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-plasma5support.h)

add_library(Plasma5Support)
add_library(Plasma::Plasma5Support ALIAS Plasma5Support)

qt_extract_metatypes(Plasma5Support)

set_target_properties(Plasma5Support PROPERTIES
    VERSION     ${PLASMA5SUPPORT_VERSION}
    SOVERSION   ${PLASMA5SUPPORT_SOVERSION}
    EXPORT_NAME Plasma5Support
)

########### next target ###############
target_sources(Plasma5Support PRIVATE
#global
    plasma5support.cpp
    pluginloader.cpp
    version.cpp

#Dataengines, services
    datacontainer.cpp
    dataengine.cpp
    dataengineconsumer.cpp
    service.cpp
    servicejob.cpp
    private/datacontainer_p.cpp
    private/dataenginemanager.cpp
    private/storage.cpp
    private/storagethread.cpp
)

ecm_qt_declare_logging_category(Plasma5Support
    HEADER debug_p.h
    IDENTIFIER LOG_PLASMA5SUPPORT
    CATEGORY_NAME kf.plasma5support.core
    OLD_CATEGORY_NAMES org.kde.plasma5support
    DESCRIPTION "Plasma5Support Core lib"
    EXPORT PLASMA5SUPPORT
)

ecm_generate_export_header(Plasma5Support
    EXPORT_FILE_NAME plasma5support/plasma5support_export.h
    BASE_NAME Plasma5Support
    GROUP_BASE_NAME KF
    VERSION ${PROJECT_VERSION}
    DEPRECATED_BASE_VERSION 0
    DEPRECATION_VERSIONS 5.6 5.19 5.28 5.30 5.36 5.46 5.67 5.77 5.78 5.81
)

target_link_libraries(Plasma5Support
PUBLIC
    KF6::ConfigCore #KConfigGroup
    KF6::CoreAddons
PRIVATE
    Qt::Gui
    Qt::Sql
    Qt::Widgets
    KF6::I18n
    KF6::ConfigGui
    KF6::CoreAddons
)

set(Plasma5Support_BUILD_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/Plasma5Support
)
target_include_directories(Plasma5Support PUBLIC
  "$<BUILD_INTERFACE:${Plasma5Support_BUILD_INCLUDE_DIRS}>"
)
target_include_directories(Plasma5Support INTERFACE
  "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR};${KDE_INSTALL_INCLUDEDIR}/Plasma5Support>"
)

set_target_properties(Plasma5Support PROPERTIES
                                VERSION ${PLASMA5SUPPORT_VERSION}
                                SOVERSION ${PLASMA5SUPPORT_SOVERSION}
                                EXPORT_NAME Plasma5Support
                      )

########### install files ###############
ecm_generate_headers(Plasma5Support_CamelCase_HEADERS
    HEADER_NAMES
        DataContainer
        DataEngine
        DataEngineConsumer
        PluginLoader
        Service
        ServiceJob
        Plasma5Support
    REQUIRED_HEADERS Plasma5Support_HEADERS
    PREFIX Plasma5Support
)

set(Plasma5Support_HEADERS
    ${Plasma5Support_HEADERS}
    version.h
)

install(FILES
        ${Plasma5Support_HEADERS}
        ${CMAKE_CURRENT_BINARY_DIR}/plasma5support/plasma5support_export.h
        DESTINATION ${KDE_INSTALL_INCLUDEDIR}/Plasma5Support/plasma5support COMPONENT Devel)

install(FILES ${Plasma5Support_CamelCase_HEADERS}
        DESTINATION ${KDE_INSTALL_INCLUDEDIR}/Plasma5Support/Plasma5Support COMPONENT Devel)

install(FILES data/operations/dataengineservice.operations DESTINATION ${PLASMA5SUPPORT_DATA_INSTALL_DIR}/services)
install(FILES data/operations/plasmoidservice.operations DESTINATION ${PLASMA5SUPPORT_DATA_INSTALL_DIR}/services)
install(FILES data/operations/storage.operations DESTINATION ${PLASMA5SUPPORT_DATA_INSTALL_DIR}/services)

install(TARGETS Plasma5Support EXPORT Plasma5SupportTargets ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

if(BUILD_QCH)
    ecm_add_qch(
        Plasma5Support_QCH
        NAME Plasma5Support
        BASE_NAME Plasma5Support
        VERSION ${PROJECT_VERSION}
        ORG_DOMAIN org.kde
        SOURCES # using only public headers, to cover only public API
            ${Plasma5Support_HEADERS}
            ${PlasmaScripting_HEADERS}
            Mainpage.dox
        MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        LINK_QCHS
            Qt6Gui_QCH
            KF6Service_QCH
            KF6Package_QCH
        INCLUDE_DIRS
            ${Plasma5Support_BUILD_INCLUDE_DIRS}
        BLANK_MACROS
            PLASMA5SUPPORT_EXPORT
            PLASMA5SUPPORT_DEPRECATED
            PLASMA5SUPPORT_DEPRECATED_EXPORT
            "PLASMA5SUPPORT_DEPRECATED_VERSION(x, y, t)"
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
    )
endif()
