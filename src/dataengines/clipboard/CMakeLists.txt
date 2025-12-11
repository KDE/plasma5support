# Plasma Data Engine
set(plasma_engine_clipboard_SRCS clipboardengine.cpp clipboardengine.h clipboardservice.cpp clipboardservice.h clipboardjob.cpp clipboardjob.h)
kcoreaddons_add_plugin(plasma_engine_clipboard SOURCES ${plasma_engine_clipboard_SRCS} INSTALL_NAMESPACE "plasma5support/dataengine")
target_link_libraries(plasma_engine_clipboard
        KF6::Prison
        PW::LibKlipper
        Plasma::Plasma5Support
)

install(FILES org.kde.plasma.clipboard.operations DESTINATION ${PLASMA5SUPPORT_DATA_INSTALL_DIR}/services)
