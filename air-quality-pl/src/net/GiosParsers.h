#pragma once

#include <QJsonDocument>
#include <QUrl>
#include <QVariantList>

namespace GiosParsers {

/**
 * Represents one parsed page from the stations endpoint.
 */
struct StationsPage {
  QVariantList stations;
  QString nextUrl;
  bool reachedEnd = true;
};

/**
 * Represents one parsed measurements series for a sensor.
 */
struct MeasurementSeries {
  QString paramCode;
  QVariantList points;
};

/**
 * Parses one stations payload page.
 * Throws std::runtime_error when the payload shape is invalid.
 */
StationsPage parseStationsPage(const QJsonDocument& doc, const QUrl& currentUrl);

/**
 * Parses a sensors payload.
 * Throws std::runtime_error when the payload shape is invalid.
 */
QVariantList parseSensors(const QJsonDocument& doc);

/**
 * Parses a measurements payload.
 * Throws std::runtime_error when the payload shape is invalid.
 */
MeasurementSeries parseMeasurements(const QJsonDocument& doc);

}  // namespace GiosParsers
