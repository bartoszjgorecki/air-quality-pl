QT += core gui qml quick quickcontrols2 network positioning location charts multimedia
CONFIG += c++17
TEMPLATE = app
TARGET = air_quality_pl

SOURCES += \
  src/main.cpp \
  src/app/AppController.cpp \
  src/storage/LocalDb.cpp \
  src/analysis/Analyzer.cpp

HEADERS += \
  src/app/AppController.h \
  src/storage/LocalDb.h \
  src/analysis/Analyzer.h \
  src/model/Types.h

RESOURCES += qml.qrc