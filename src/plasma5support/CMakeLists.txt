
if(KDE_PLATFORM_FEATURE_BINARY_COMPATIBLE_FEATURE_REDUCTION)
    set(PLASMA5SUPPORT_NO_PACKAGE_EXTRADATA TRUE)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-plasma5support.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-plasma5support.h)

add_library(KF6Plasma5Support)
add_library(KF6::Plasma5Support ALIAS KF6Plasma5Support)

set_target_properties(KF6Plasma5Support PROPERTIES
    VERSION     ${PLASMA5SUPPORT_VERSION}
    SOVERSION   ${PLASMA5SUPPORT_SOVERSION}
    EXPORT_NAME Plasma5Support
)

########### next target ###############
target_sources(KF6Plasma5Support PRIVATE
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

ecm_qt_declare_logging_category(KF6Plasma5Support
    HEADER debug_p.h
    IDENTIFIER LOG_PLASMA5SUPPORT
    CATEGORY_NAME kf.plasma5support.core
    OLD_CATEGORY_NAMES org.kde.plasma5support
    DESCRIPTION "Plasma5Support Core lib"
    EXPORT PLASMA5SUPPORT
)

ecm_generate_export_header(KF6Plasma5Support
    EXPORT_FILE_NAME plasma5support/plasma5support_export.h
    BASE_NAME Plasma5Support
    GROUP_BASE_NAME KF
    VERSION ${KF_VERSION}
    DEPRECATED_BASE_VERSION 0
    DEPRECATION_VERSIONS 5.6 5.19 5.28 5.30 5.36 5.46 5.67 5.77 5.78 5.81
)

target_link_libraries(KF6Plasma5Support
PUBLIC
    KF6::Service # For kplugininfo.h and kservice.h
    KF6::KIOCore    #ServiceJob
    KF6::ConfigCore #KConfigGroup
    KF6::CoreAddons
PRIVATE
    Qt::Sql
    KF6::I18n
    KF6::CoreAddons
    KF6::KIOWidgets  #KIO::JobUiDelegate
    KF6::XmlGui   #KConfigLoader
    KF6::Package
)

set(Plasma5Support_BUILD_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/Plasma5Support
)
target_include_directories(KF6Plasma5Support PUBLIC
  "$<BUILD_INTERFACE:${Plasma5Support_BUILD_INCLUDE_DIRS}>"
)
target_include_directories(KF6Plasma5Support INTERFACE
  "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF};${KDE_INSTALL_INCLUDEDIR_KF}/Plasma5Support>"
)

set_target_properties(KF6Plasma5Support PROPERTIES
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
        DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/plasma5support COMPONENT Devel)

install(FILES ${Plasma5Support_CamelCase_HEADERS}
        DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/Plasma5Support COMPONENT Devel)


install(FILES
   data/servicetypes/plasma5support-dataengine.desktop
   data/servicetypes/plasma5support-service.desktop
   DESTINATION ${KDE_INSTALL_KSERVICETYPESDIR})

install(FILES data/operations/dataengineservice.operations DESTINATION ${PLASMA5SUPPORT_DATA_INSTALL_DIR}/services)
install(FILES data/operations/plasmoidservice.operations DESTINATION ${PLASMA5SUPPORT_DATA_INSTALL_DIR}/services)
install(FILES data/operations/storage.operations DESTINATION ${PLASMA5SUPPORT_DATA_INSTALL_DIR}/services)

install(TARGETS KF6Plasma5Support EXPORT KF6Plasma5SupportTargets ${KF6_INSTALL_TARGETS_DEFAULT_ARGS})

if(BUILD_QCH)
    ecm_add_qch(
        KF6Plasma5Support_QCH
        NAME Plasma5Support
        BASE_NAME KF6Plasma5Support
        VERSION ${KF_VERSION}
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
