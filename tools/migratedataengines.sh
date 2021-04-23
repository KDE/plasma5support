 #!/bin/sh

find . -name "*.h" -o -name "*.cpp" | xargs perl -p -i -e "s/Plasma(\/|::)DataContainer/Plasma5Support\\1DataContainer/g"
find . -name "*.h" -o -name "*.cpp" | xargs perl -p -i -e "s/Plasma(\/|::)DataEngine/Plasma5Support\\1DataEngine/g"

find . -name "*.h" -o -name "*.cpp" | xargs perl -p -i -e "s/K_EXPORT_PLASMA_DATAENGINE_WITH_JSON/K_EXPORT_PLASMA5SUPPORT_DATAENGINE_WITH_JSON/g"

