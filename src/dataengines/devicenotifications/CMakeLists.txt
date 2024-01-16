add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_devicenotifications\")

set(device_notifications_engine_SRCS
    devicenotificationsengine.cpp
    ksolidnotify.cpp
    devicenotificationsengine.h
    ksolidnotify.h
)

add_library(plasma_engine_devicenotifications MODULE ${device_notifications_engine_SRCS})

target_link_libraries(plasma_engine_devicenotifications
    KF6::I18n
    KF6::Notifications
    KF6::Service
    KF6::Solid
    KSysGuard::ProcessCore
    Plasma::Plasma
    Plasma::Plasma5Support
)

install(TARGETS plasma_engine_devicenotifications DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma5support/dataengine)
