

set(plasma5support_SRCS
    plasma5supportplugin.cpp
    datamodel.cpp
    datasource.cpp
    serviceoperationstatus.cpp
    )

add_library(plasma5supportplugin SHARED ${plasma5support_SRCS})
target_link_libraries(plasma5supportplugin
        Qt5::Quick
        Qt5::Qml
        KF5::I18n
        KF5::Service #for kplugininfo.h
        KF5::Plasma5Support
)

install(TARGETS plasma5supportplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/plasma5support)
install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/plasma5support)

ecm_generate_qmltypes(org.kde.plasma.plasma5support 2.0 DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/plasma5support)

#also "install" to $BUILD_DIR/bin for unit tests
add_custom_target(copy ALL DEPENDS plasma5supportplugin)
add_custom_command(TARGET copy POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E
                        copy ${CMAKE_CURRENT_SOURCE_DIR}/qmldir ${CMAKE_BINARY_DIR}/bin/org/kde/plasma/plasma5support/qmldir)
add_custom_command(TARGET copy POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E
                    copy $<TARGET_FILE:plasma5supportplugin> ${CMAKE_BINARY_DIR}/bin/org/kde/plasma/plasma5support/)

