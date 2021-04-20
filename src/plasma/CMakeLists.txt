
# This option should be removed, or moved down as far as possible.
# That means porting the existing frameworks to the CMake automoc
# feature. Porting is mostly removing explicit moc includes, and
# leaving the ones which are truly needed (ie, if you remove
# them, the build fails).
#set(CMAKE_AUTOMOC_RELAXED_MODE ON)

if(KDE_PLATFORM_FEATURE_BINARY_COMPATIBLE_FEATURE_REDUCTION)
    set(PLASMA_NO_PACKAGE_EXTRADATA TRUE)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-plasma5support.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-plasma5support.h)

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=1209)

########### next target ###############

set(Plasma5Support_LIB_SRCS
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

if(HAVE_X11)
    set(Plasma5Support_LIB_SRCS ${Plasma5Support_LIB_SRCS} private/effectwatcher.cpp)
endif()

kconfig_add_kcfg_files(Plasma5Support_LIB_SRCS data/kconfigxt/libplasma-theme-global.kcfgc)

ecm_qt_declare_logging_category(Plasma5Support_LIB_SRCS
    HEADER debug_p.h
    IDENTIFIER LOG_PLASMA
    CATEGORY_NAME kf.plasma5support.core
    OLD_CATEGORY_NAMES org.kde.plasma5support
    DESCRIPTION "Plasma Core lib"
    EXPORT PLASMA5SUPPORT
)

add_library(KF5Plasma5Support ${Plasma5Support_LIB_SRCS})
add_library(KF5::Plasma5Support ALIAS KF5Plasma5Support)
ecm_generate_export_header(KF5Plasma5Support
    EXPORT_FILE_NAME plasma/plasma5support_export.h
    BASE_NAME Plasma5Support
    GROUP_BASE_NAME KF
    VERSION ${KF_VERSION}
    DEPRECATED_BASE_VERSION 0
    DEPRECATION_VERSIONS 5.6 5.19 5.28 5.30 5.36 5.46 5.67 5.77 5.78 5.81
)
# TODO: add support for EXCLUDE_DEPRECATED_BEFORE_AND_AT to all Plasma libs
# needs fixing of undeprecated API being still implemented using own deprecated API

if(HAVE_X11)
    set(PLASMA_EXTRA_LIBS ${PLASMA_EXTRA_LIBS} Qt5::X11Extras ${X11_LIBRARIES} XCB::XCB)
endif()

target_link_libraries(KF5Plasma5Support
PUBLIC
    KF5::Service # For kplugininfo.h and kservice.h
    Qt5::Gui
    KF5::Package
PRIVATE
    Qt5::Sql
    Qt5::DBus
    KF5::Archive
    KF5::GuiAddons #kimagecache
    KF5::I18n
    KF5::KIOCore    #ServiceJob
    KF5::KIOWidgets  #KIO::JobUiDelegate
    KF5::WindowSystem #compositingActive
    KF5::Declarative #runtimePlatform
    KF5::XmlGui   #KActionCollection
    KF5::GlobalAccel #Applet::setGlobalShortcut
    KF5::Notifications
    KF5::IconThemes
    KF5::Plasma
    ${PLASMA_EXTRA_LIBS}
)

set(Plasma_BUILD_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/Plasma
)
target_include_directories(KF5Plasma5Support PUBLIC
  "$<BUILD_INTERFACE:${Plasma_BUILD_INCLUDE_DIRS}>"
)
target_include_directories(KF5Plasma5Support INTERFACE
  "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF5};${KDE_INSTALL_INCLUDEDIR_KF5}/Plasma>"
)

set_target_properties(KF5Plasma5Support PROPERTIES
                                VERSION ${PLASMA_VERSION_STRING}
                                SOVERSION ${PLASMA_SOVERSION}
                                EXPORT_NAME Plasma
                      )

########### install files ###############
ecm_generate_headers(Plasma_CamelCase_HEADERS
    HEADER_NAMES
        DataContainer
        DataEngine
        DataEngineConsumer
        PluginLoader
        Service
        ServiceJob
        Plasma5Support
    REQUIRED_HEADERS Plasma_HEADERS
    PREFIX Plasma
)

set(Plasma_HEADERS
    ${Plasma_HEADERS}
    version.h
)

install(FILES
        ${Plasma_HEADERS}
        ${CMAKE_CURRENT_BINARY_DIR}/plasma/plasma_export.h
        DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/plasma5support COMPONENT Devel)

install(FILES ${Plasma_CamelCase_HEADERS}
        DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/Plasma5Support COMPONENT Devel)


install(FILES
   data/servicetypes/plasma-dataengine.desktop
   data/servicetypes/plasma-service.desktop
   DESTINATION ${KDE_INSTALL_KSERVICETYPES5DIR})

install(FILES data/operations/dataengineservice.operations DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)
install(FILES data/operations/plasmoidservice.operations DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)
install(FILES data/operations/storage.operations DESTINATION ${PLASMA_DATA_INSTALL_DIR}/services)

install(TARGETS KF5Plasma5Support EXPORT KF5Plasma5SupportTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

if(BUILD_QCH)
    ecm_add_qch(
        KF5Plasma5Support_QCH
        NAME Plasma
        BASE_NAME KF5Plasma5Support
        VERSION ${KF_VERSION}
        ORG_DOMAIN org.kde
        SOURCES # using only public headers, to cover only public API
            ${Plasma_HEADERS}
            ${PlasmaScripting_HEADERS}
            Mainpage.dox
        MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
        LINK_QCHS
            Qt5Gui_QCH
            KF5Service_QCH
            KF5Package_QCH
        INCLUDE_DIRS
            ${Plasma_BUILD_INCLUDE_DIRS}
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
