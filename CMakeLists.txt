cmake_minimum_required(VERSION 3.16)

set(KF_VERSION "5.240.0") # handled by release scripts
set(KF_DEP_VERSION "5.240.0") # handled by release scripts
project(Plasma5Support VERSION ${KF_VERSION})

# ECM setup
include(FeatureSummary)
find_package(ECM 5.240.0  NO_MODULE)
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
include(ECMMarkNonGuiExecutable)
include(ECMDeprecationSettings)
include(ECMQmlModule)

option(BUILD_QCH "Build API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)" OFF)
add_feature_info(QCH ${BUILD_QCH} "API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)")

ecm_setup_version(PROJECT
                  VARIABLE_PREFIX PLASMA5SUPPORT
                  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/plasma5support_version.h"
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KF6Plasma5SupportConfigVersion.cmake"
                  SOVERSION 6)

add_definitions(-DQT_NO_FOREACH)
################# now find all used packages #################

set (REQUIRED_QT_VERSION 6.4.0)

find_package(Qt${QT_MAJOR_VERSION} ${REQUIRED_QT_VERSION} REQUIRED NO_MODULE COMPONENTS Core Quick Sql Qml)

find_package(KF6 ${KF_DEP_VERSION} REQUIRED
    COMPONENTS
        Config
        ConfigWidgets
        CoreAddons
        I18n
        KIO
        Package
        Service
        XmlGui
    OPTIONAL_COMPONENTS
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

include(KF6Plasma5SupportMacros.cmake)

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
#    add_subdirectory(autotests)
endif()

################ create Plasma5SupportConfig.cmake and install it ###########################

# create a Config.cmake and a ConfigVersion.cmake file and install them

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KF6Plasma5Support")

if (BUILD_QCH)
    ecm_install_qch_export(
        TARGETS KF6Plasma5Support_QCH
        FILE KF6Plasma5SupportQchTargets.cmake
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel
    )
    set(PACKAGE_INCLUDE_QCHTARGETS "include(\"\${CMAKE_CURRENT_LIST_DIR}/KF6Plasma5SupportQchTargets.cmake\")")
endif()

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/KF6Plasma5SupportConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/KF6Plasma5SupportConfig.cmake"
  INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
  PATH_VARS  CMAKE_INSTALL_PREFIX
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/KF6Plasma5SupportConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/KF6Plasma5SupportConfigVersion.cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/KF6Plasma5SupportMacros.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT KF6Plasma5SupportTargets
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        FILE KF6Plasma5SupportTargets.cmake
        NAMESPACE KF6::
        COMPONENT Devel)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plasma5support_version.h
        DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF} COMPONENT Devel )

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
