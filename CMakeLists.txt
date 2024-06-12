cmake_minimum_required(VERSION 3.16)
set(PROJECT_VERSION "6.1.0")
project(Plasma5Support VERSION ${PROJECT_VERSION})

set(PROJECT_DEP_VERSION "6.1.0")
set(QT_MIN_VERSION "6.6.0")
set(KF6_MIN_VERSION "6.2.0")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

include(FeatureSummary)
include(KDEInstallDirs)
include(KDEClangFormat)
include(KDECMakeSettings)
include(KDEGitCommitHooks)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMGenerateExportHeader)
include(ECMGenerateHeaders)
include(CMakePackageConfigHelpers)
include(ECMSetupVersion)
include(ECMQtDeclareLoggingCategory)
include(ECMAddQch)
include(KDEPackageAppTemplates)
include(ECMGenerateQmlTypes)
include(ECMSetupQtPluginMacroNames)
include(ECMMarkNonGuiExecutable)
include(ECMDeprecationSettings)
include(ECMQmlModule)

option(BUILD_QCH "Build API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)" OFF)
add_feature_info(QCH ${BUILD_QCH} "API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)")

ecm_setup_version(PROJECT
                  VARIABLE_PREFIX PLASMA5SUPPORT
                  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/plasma5support_version.h"
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/Plasma5SupportConfigVersion.cmake"
                  SOVERSION 6)

add_definitions(-DQT_NO_FOREACH)
################# now find all used packages #################

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Core Gui Quick Sql Qml Widgets)

find_package(KF6 ${KF_DEP_VERSION} REQUIRED
    COMPONENTS
        Config
        CoreAddons
        GuiAddons
        I18n
        Notifications
        Solid
)
find_package(KSysGuard CONFIG REQUIRED) # devicenotifications

#########################################################################

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050e00) # must port away from qmlRegisterInterface before upgrading this to 050f00
add_definitions(-DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x055100)

#########################################################################

include(Plasma5SupportMacros.cmake)

option(BUILD_COVERAGE "Build Plasma5Support Frameworks with gcov support" OFF)

if(BUILD_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lgcov")
endif()

# make plasma5support_version.h available
include_directories(${CMAKE_CURRENT_BINARY_DIR})

################# list the subdirectories #################

add_definitions(-DTRANSLATION_DOMAIN=\"libplasma5support\")
if (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/po")
    ki18n_install(po)
endif()
add_subdirectory(src)

if (BUILD_TESTING)
    find_package(Qt6Test ${REQUIRED_QT_VERSION} REQUIRED)
    add_subdirectory(autotests)
endif()

################ create Plasma5SupportConfig.cmake and install it ###########################

# create a Config.cmake and a ConfigVersion.cmake file and install them

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/Plasma5Support")

if (BUILD_QCH)
    ecm_install_qch_export(
        TARGETS Plasma5Support_QCH
        FILE Plasma5SupportQchTargets.cmake
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel
    )
    set(PACKAGE_INCLUDE_QCHTARGETS "include(\"\${CMAKE_CURRENT_LIST_DIR}/Plasma5SupportQchTargets.cmake\")")
endif()

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/Plasma5SupportConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/Plasma5SupportConfig.cmake"
  INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
  PATH_VARS  CMAKE_INSTALL_PREFIX
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/Plasma5SupportConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/Plasma5SupportConfigVersion.cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/Plasma5SupportMacros.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT Plasma5SupportTargets
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        FILE Plasma5SupportTargets.cmake
        NAMESPACE Plasma::
        COMPONENT Devel)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plasma5support_version.h
        DESTINATION ${KDE_INSTALL_INCLUDEDIR}/Plasma5Support COMPONENT Devel )

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
