cmake_minimum_required(VERSION 3.5)

set(KF_VERSION "5.82.0") # handled by release scripts
set(KF_DEP_VERSION "5.81.0") # handled by release scripts
project(Plasma VERSION ${KF_VERSION})

# ECM setup
include(FeatureSummary)
find_package(ECM 5.81.0  NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://commits.kde.org/extra-cmake-modules")
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND FATAL_ON_MISSING_REQUIRED_PACKAGES)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEGitCommitHooks)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)

include(ECMGenerateExportHeader)
include(ECMGenerateHeaders)
include(CMakePackageConfigHelpers)
include(ECMSetupVersion)
include(ECMQtDeclareLoggingCategory)
include(ECMAddQch)
include(KDEPackageAppTemplates)
include(ECMGenerateQmlTypes)
include(ECMSetupQtPluginMacroNames)

option(BUILD_QCH "Build API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)" OFF)
add_feature_info(QCH ${BUILD_QCH} "API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)")

ecm_setup_version(PROJECT
                  VARIABLE_PREFIX PLASMA5SUPPORT
                  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/plasma5support_version.h"
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF5Plasma5SupportConfigVersion.cmake"
                  SOVERSION 5)

add_definitions(-DQT_NO_FOREACH)
################# now find all used packages #################

set (REQUIRED_QT_VERSION 5.15.0)

find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED NO_MODULE COMPONENTS Quick Gui Sql Qml)

find_package(KF5 ${KF_DEP_VERSION} REQUIRED
    COMPONENTS
        Archive
        Config
        ConfigWidgets
        CoreAddons
        DBusAddons
        Declarative
        GlobalAccel
        GuiAddons
        I18n
        IconThemes
        KIO
        Service
        WindowSystem
        XmlGui
        Notifications
        Package
        Kirigami2
        Plasma
    OPTIONAL_COMPONENTS
        Wayland
        DocTools
)



#########################################################################

ecm_setup_qtplugin_macro_names(
    JSON_NONE
        "K_EXPORT_PLASMA5SUPPORT_SERVICE"
        "K_EXPORT_PLASMA5SUPPORT_DATAENGINE"
    JSON_ARG3
        "K_EXPORT_PLASMA5SUPPORT_SERVICE_WITH_JSON"
        "K_EXPORT_PLASMA5SUPPORT_DATAENGINE_WITH_JSON"
    CONFIG_CODE_VARIABLE
        PACKAGE_SETUP_AUTOMOC_VARIABLES
)
add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050e00) # must port away from qmlRegisterInterface before upgrading this to 050f00
add_definitions(-DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x055100)

#add_definitions(-Wno-deprecated)

#########################################################################


option(BUILD_COVERAGE "Build Plasma Frameworks with gcov support" OFF)

if(BUILD_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lgcov")
endif()

# make plasma_version.h available
include_directories(${CMAKE_CURRENT_BINARY_DIR})

################# list the subdirectories #################

add_definitions(-DTRANSLATION_DOMAIN=\"libplasma5support\")
if (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/po")
    ki18n_install(po)
endif()
add_subdirectory(src)


if (BUILD_TESTING)
#    add_subdirectory(autotests)
endif()

################ create PlasmaConfig.cmake and install it ###########################

# create a Config.cmake and a ConfigVersion.cmake file and install them

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KF5PlasmaSupport")

if (BUILD_QCH)
    ecm_install_qch_export(
        TARGETS KF5Plasma_QCH
        FILE KF5PlasmaQchTargets.cmake
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel
    )
    set(PACKAGE_INCLUDE_QCHTARGETS "include(\"\${CMAKE_CURRENT_LIST_DIR}/KF5PlasmaQchTargets.cmake\")")
endif()

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/KF5Plasma5SupportConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/KF5Plasma5SupportConfig.cmake"
  INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
  PATH_VARS  CMAKE_INSTALL_PREFIX
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/KF5Plasma5SupportConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/KF5Plasma5SupportConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT KF5Plasma5SupportTargets
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        FILE KF5Plasma5SupportTargets.cmake
        NAMESPACE KF5::
        COMPONENT Devel)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plasma5support_version.h
        DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5} COMPONENT Devel )

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
