# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

remove_definitions(-DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_CAST_TO_ASCII)
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)

if (TARGET KSysGuard::ProcessCore)
    add_subdirectory(devicenotifications)
endif()
add_subdirectory(favicons)
if (KF6NetworkManagerQt_FOUND)
    add_subdirectory(geolocation)
endif ()
if (PlasmaActivities_FOUND)
    add_subdirectory(activities)
endif ()
add_subdirectory(keystate)
add_subdirectory(hotplug)
add_subdirectory(soliddevice)
add_subdirectory(powermanagement)
add_subdirectory(packagekit)
add_subdirectory(mouse)
add_subdirectory(filebrowser)
add_subdirectory(places)
add_subdirectory(apps)
add_subdirectory(weather)
