find_package(ECM 5.240.0 CONFIG REQUIRED)
include(${ECM_KDE_MODULE_DIR}/KDEInstallDirs.cmake)

set(PLASMA5SUPPORT_RELATIVE_DATA_INSTALL_DIR "plasma5support")
set(PLASMA5SUPPORT_DATA_INSTALL_DIR "${KDE_INSTALL_DATADIR}/${PLASMA5SUPPORT_RELATIVE_DATA_INSTALL_DIR}")