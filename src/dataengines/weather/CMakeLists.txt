add_definitions(-DTRANSLATION_DOMAIN=\"plasma_engine_weather\")
remove_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x50f02)

add_subdirectory(ions)

set(weather_SRCS weatherengine.cpp weatherengine.h)
ecm_qt_declare_logging_category(weather_SRCS
    HEADER weatherenginedebug.h
    IDENTIFIER WEATHER
    CATEGORY_NAME kde.dataengine.weather
    DEFAULT_SEVERITY Info
    DESCRIPTION "DataEngine weather engine"
    EXPORT PLASMAWORKSPACE
)

kcoreaddons_add_plugin(plasma_engine_weather SOURCES ${weather_SRCS} INSTALL_NAMESPACE plasma5support/dataengine)
target_compile_definitions(plasma_engine_weather PRIVATE -DQT_DISABLE_DEPRECATED_BEFORE=0x050e00) # needed for QNetworkConfigurationManager

target_link_libraries (plasma_engine_weather
            KF6::KIOCore
            KF6::Solid
            Plasma::Plasma5Support
            KF6::Service
            Qt::Network
            weather_ion)
