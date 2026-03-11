cmake_minimum_required(VERSION 3.21)
project(air_quality_pl LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Główny projekt korzysta z Qt Quick do interfejsu, Qt Charts do wykresu,
# Qt Location do mapy oraz Qt Concurrent do parsowania JSON poza wątkiem UI.
find_package(Qt6 REQUIRED COMPONENTS
  Core Gui Widgets Qml Quick QuickControls2 Network Positioning Location Charts Multimedia Test Concurrent
)

qt_standard_project_setup()
enable_testing()

# Aplikacja główna składa się z kontrolera, klienta API, lokalnej bazy
# i modułu analizy danych.
qt_add_executable(air_quality_pl
  src/main.cpp
  src/app/AppController.cpp
  src/storage/LocalDb.cpp
  src/analysis/Analyzer.cpp
  src/net/GiosClient.cpp
  src/net/GiosParsers.cpp
)

target_include_directories(air_quality_pl PRIVATE src)

qt_add_resources(QML_RESOURCES qml.qrc)
target_sources(air_quality_pl PRIVATE ${QML_RESOURCES})

# Linkujemy pełny zestaw modułów używany przez UI, sieć, mapę i wykres.
target_link_libraries(air_quality_pl PRIVATE
  Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Qml Qt6::Quick Qt6::QuickControls2
  Qt6::Network Qt6::Positioning Qt6::Location Qt6::Charts Qt6::Multimedia Qt6::Concurrent
)

# Testy jednostkowe obejmują parsery, lokalną bazę i analizator statystyk.
qt_add_executable(air_quality_pl_tests
  tests/test_core.cpp
  src/storage/LocalDb.cpp
  src/analysis/Analyzer.cpp
  src/net/GiosParsers.cpp
)

target_include_directories(air_quality_pl_tests PRIVATE src)

target_link_libraries(air_quality_pl_tests PRIVATE
  Qt6::Core Qt6::Test
)

add_test(NAME air_quality_pl_tests COMMAND air_quality_pl_tests)
