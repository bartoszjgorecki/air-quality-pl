#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include "app/AppController.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  QQuickStyle::setStyle("Basic");

  QQmlApplicationEngine engine;

  AppController controller;
  engine.rootContext()->setContextProperty("App", &controller);

  engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
  if (engine.rootObjects().isEmpty()) return -1;

  return app.exec();
}
