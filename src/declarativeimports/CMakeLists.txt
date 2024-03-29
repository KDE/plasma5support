
ecm_add_qml_module(plasma5supportplugin URI "org.kde.plasma.plasma5support" VERSION 2.0 DEPENDENCIES QtCore QtQuick GENERATE_PLUGIN_SOURCE)

target_sources(plasma5supportplugin PRIVATE
    datamodel.cpp
    datasource.cpp
    serviceoperationstatus.cpp
    types.h
    )


target_link_libraries(plasma5supportplugin PRIVATE
        Qt::Quick
        Qt::Qml
        KF6::ConfigCore
        KF6::I18n
        Plasma::Plasma5Support
        KF6::CoreAddons
)

ecm_finalize_qml_module(plasma5supportplugin DESTINATION ${KDE_INSTALL_QMLDIR})
