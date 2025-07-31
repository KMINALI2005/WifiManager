cmake_minimum_required(VERSION 3.16)
project(WifiManager)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# البحث عن Qt6 مع Charts
find_package(Qt6 6.2 REQUIRED COMPONENTS Core Widgets Network Charts)

# تعريف الملفات المصدر
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/wifimanager.cpp
    src/networkstats.cpp
)

set(HEADERS
    src/mainwindow.h
    src/wifimanager.h
    src/networkstats.h
)

# إنشاء التطبيق التنفيذي
qt6_add_executable(WifiManager
    ${SOURCES}
    ${HEADERS}
)

# ربط المكتبات
target_link_libraries(WifiManager
    PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Charts
)

# تضمين المجلدات
target_include_directories(WifiManager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Finalize executable
qt6_finalize_executable(WifiManager)
