add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_powermanagement\")

set(powermanagement_engine_SRCS
    powermanagementengine.cpp
    powermanagementjob.cpp
    powermanagementservice.cpp
    powermanagementengine.h
    powermanagementjob.h
    powermanagementservice.h
)

kcoreaddons_add_plugin(plasma_engine_powermanagement SOURCES ${powermanagement_engine_SRCS} INSTALL_NAMESPACE plasma5support/dataengine)

target_link_libraries(plasma_engine_powermanagement
    KF6::Solid
    KF6::IdleTime
    KF6::CoreAddons
    KF6::I18n
    KF6::Service
    Qt::DBus
    Qt::Gui
    Plasma::Plasma5Support
)

install(FILES powermanagementservice.operations DESTINATION ${PLASMA5SUPPORT_DATA_INSTALL_DIR}/services)
