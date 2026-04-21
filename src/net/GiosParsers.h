#pragma once

#include <QJsonDocument>
#include <QUrl>
#include <QVariantList>

namespace GiosParsers {

// Parsery w tej przestrzeni nazw uodparniają aplikację na różne kształty
// odpowiedzi zwracanych przez publiczne API GIOS.

/**
 * Reprezentuje jedną sparsowaną stronę odpowiedzi endpointu stacji.
 */
struct StationsPage {
  /// Znormalizowana lista stacji z bieżącej strony odpowiedzi.
  QVariantList stations;
  /// Adres URL kolejnej strony, jeśli API go zwróciło.
  QString nextUrl;
  /// Informuje, czy dalsze pobieranie stron nie jest już potrzebne.
  bool reachedEnd = true;
};

/**
 * Reprezentuje jedną sparsowaną serię pomiarową dla sensora.
 */
struct MeasurementSeries {
  /// Rozpoznany kod parametru, na przykład PM10 albo NO2.
  QString paramCode;
  /// Punkty serii gotowe do przekazania do dalszej części aplikacji.
  QVariantList points;
};

/**
 * Parsuje jedną stronę odpowiedzi ze stacjami.
 * Zgłasza `std::runtime_error`, gdy kształt JSON jest niepoprawny.
 */
StationsPage parseStationsPage(const QJsonDocument& doc, const QUrl& currentUrl);

/**
 * Parsuje odpowiedź zawierającą listę sensorów.
 * Zgłasza `std::runtime_error`, gdy kształt JSON jest niepoprawny.
 */
QVariantList parseSensors(const QJsonDocument& doc);

/**
 * Parsuje odpowiedź zawierającą listę pomiarów.
 * Zgłasza `std::runtime_error`, gdy kształt JSON jest niepoprawny.
 */
MeasurementSeries parseMeasurements(const QJsonDocument& doc);

}  // namespace GiosParsers
