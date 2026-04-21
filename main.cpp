#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "app/AppController.h"

int main(int argc, char *argv[]) {
  // To jest starszy punkt wejścia dla wariantu opartego wyłącznie o QGuiApplication.
  // Główna konfiguracja używana przez CMake znajduje się w src/main.cpp.
  QGuiApplication app(argc, argv);

  QQmlApplicationEngine engine;

  // Udostępniamy kontroler do QML jako pojedynczy obiekt aplikacyjny.
  AppController controller;
  engine.rootContext()->setContextProperty("App", &controller);

  engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
  if (engine.rootObjects().isEmpty()) return -1;

  return app.exec();
}
