#pragma once

#include <QJsonDocument>
#include <QUrl>
#include <QVariantList>

namespace GiosParsers {

/**
 * Reprezentuje jedną sparsowaną stronę endpointu stacji.
 */
struct StationsPage {
  QVariantList stations;
  QString nextUrl;
  bool reachedEnd = true;
};

/**
 * Reprezentuje jedną serię pomiarową sparsowaną dla sensora.
 */
struct MeasurementSeries {
  QString paramCode;
  QVariantList points;
};

/**
 * Parsuje pojedynczą stronę odpowiedzi ze stacjami.
 * Rzuca std::runtime_error, gdy kształt JSON jest niepoprawny.
 */
StationsPage parseStationsPage(const QJsonDocument& doc, const QUrl& currentUrl);

/**
 * Parsuje odpowiedź z listą sensorów.
 * Rzuca std::runtime_error, gdy kształt JSON jest niepoprawny.
 */
QVariantList parseSensors(const QJsonDocument& doc);

/**
 * Parsuje odpowiedź z listą pomiarów.
 * Rzuca std::runtime_error, gdy kształt JSON jest niepoprawny.
 */
MeasurementSeries parseMeasurements(const QJsonDocument& doc);

}  // namespace GiosParsers
