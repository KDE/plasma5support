set(favicons_engine_SRCS
    favicons.cpp
    faviconprovider.cpp
    favicons.h
    faviconprovider.h
)

kcoreaddons_add_plugin(plasma_engine_favicons SOURCES ${favicons_engine_SRCS} INSTALL_NAMESPACE plasma5support/dataengine)
target_link_libraries(plasma_engine_favicons
    Plasma::Plasma5Support
    KF6::KIOCore
    Qt::Gui
)
