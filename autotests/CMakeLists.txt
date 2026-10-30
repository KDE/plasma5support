include(ECMAddTests)

ecm_add_tests(pluginloadertest.cpp datacontainertest.cpp LINK_LIBRARIES Plasma::Plasma5Support Qt6::Test)
