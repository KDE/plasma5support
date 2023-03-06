
ecm_add_qml_module(plasma5supportplugin URI "org.kde.plasma.plasma5support")

target_sources(plasma5supportplugin PRIVATE
    plasma5supportplugin.cpp
    datamodel.cpp
    datasource.cpp
    serviceoperationstatus.cpp
    )


target_link_libraries(plasma5supportplugin
        Qt${QT_MAJOR_VERSION}::Quick
        Qt${QT_MAJOR_VERSION}::Qml
        KF6::ConfigCore
        KF6::I18n
        KF6::Service #for kplugininfo.h
        KF6::Plasma5Support
        KF6::CoreAddons
)

ecm_finalize_qml_module(plasma5supportplugin DESTINATION ${KDE_INSTALL_QMLDIR})
