 #!/bin/sh

# C++ files
find . -name "*.h" -o -name "*.cpp" | xargs perl -p -i -e "s/Plasma(\/|::)DataContainer/Plasma5Support\\1DataContainer/g"
find . -name "*.h" -o -name "*.cpp" | xargs perl -p -i -e "s/Plasma(\/|::)DataEngine/Plasma5Support\\1DataEngine/g"
find . -name "*.h" -o -name "*.cpp" | xargs perl -p -i -e "s/Plasma(\/|::)DataEngineConsumer/Plasma5Support\\1DataEngineConsumer/g"

find . -name "*.h" -o -name "*.cpp" | xargs perl -p -i -e "s/Plasma(\/|::)Service/Plasma5Support\\1Service/g"
find . -name "*.h" -o -name "*.cpp" | xargs perl -p -i -e "s/Plasma(\/|::)ServiceJob/Plasma5Support\\1ServiceJob/g"

find . -name "*.h" -o -name "*.cpp" | xargs perl -p -i -e "s/K_EXPORT_PLASMA_DATAENGINE_WITH_JSON/K_EXPORT_PLASMA5SUPPORT_DATAENGINE_WITH_JSON/g"

# Desktop files
find . -name "*.desktop" | xargs perl -p -i -e "s/Plasma\/DataEngine/Plasma5Support\/DataEngine/g"

# CMake
