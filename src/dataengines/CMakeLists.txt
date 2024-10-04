# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>

remove_definitions(-DQT_NO_CAST_FROM_ASCII)
remove_definitions(-DQT_NO_CAST_TO_ASCII)
remove_definitions(-DQT_NO_CAST_FROM_BYTEARRAY)

add_subdirectory(devicenotifications)
add_subdirectory(keystate)
add_subdirectory(hotplug)
add_subdirectory(soliddevice)
add_subdirectory(powermanagement)
add_subdirectory(favicons)
