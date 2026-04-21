#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include "app/AppController.h"

// Punkt wejścia całej aplikacji.
// Ten plik ma prostą rolę: uruchomić runtime Qt i połączyć obiekt C++
// z interfejsem QML, który będzie go potem wywoływał.
int main(int argc, char *argv[]) {
  // QApplication jest potrzebny, bo aplikacja korzysta równocześnie z Qt Quick
  // i modułów widżetowych/wykresowych dostępnych w Qt.
  QApplication app(argc, argv);

  // Wybieramy prosty styl kontrolek, a właściwy wygląd i tak nadajemy w QML.
  QQuickStyle::setStyle("Basic");

  QQmlApplicationEngine engine;

  // Jeden kontroler spina logikę UI, sieć, analizę i bazę lokalną.
  AppController controller;
  engine.rootContext()->setContextProperty("App", &controller);

  // Interfejs jest w zasobach qrc, więc nie zależy od bieżącej ścieżki plików.
  engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

  // Jeśli QML się nie załaduje, kończymy aplikację od razu kodem błędu.
  if (engine.rootObjects().isEmpty()) return -1;

  // Od tego miejsca sterowanie przejmuje pętla zdarzeń Qt.
  return app.exec();
}
