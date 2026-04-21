#pragma once

#include <QJsonDocument>
#include <QUrl>
#include <QVariantList>

namespace GiosParsers {

// Parsers in this namespace make the application resilient to the different
// response shapes returned by the public GIOS API.

/**
 * Represents one parsed page from the stations endpoint.
 */
struct StationsPage {
  /// Normalized list of stations from the current page.
  QVariantList stations;
  /// URL of the next page, if the API provided one.
  QString nextUrl;
  /// Whether additional page downloads are no longer necessary.
  bool reachedEnd = true;
};

/**
 * Represents one parsed measurement series for a sensor.
 */
struct MeasurementSeries {
  /// Recognized parameter code, for example PM10 or NO2.
  QString paramCode;
  /// Series points ready to be passed to the rest of the application.
  QVariantList points;
};

/**
 * Parses one page of the stations response.
 * Throws std::runtime_error when the JSON shape is invalid.
 */
StationsPage parseStationsPage(const QJsonDocument& doc, const QUrl& currentUrl);

/**
 * Parses a sensor-list response.
 * Throws std::runtime_error when the JSON shape is invalid.
 */
QVariantList parseSensors(const QJsonDocument& doc);

/**
 * Parses a measurement-list response.
 * Throws std::runtime_error when the JSON shape is invalid.
 */
MeasurementSeries parseMeasurements(const QJsonDocument& doc);

}  // namespace GiosParsers
