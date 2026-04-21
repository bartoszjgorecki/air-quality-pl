#pragma once

#include <QString>
#include <QDateTime>
#include <QVector>
#include <optional>

/**
 * Represents a single measurement point in time.
 */
struct MeasurementPoint {
  /// Timestamp of a single measurement.
  QDateTime dt;
  /// Measurement value; std::nullopt means that the source data was missing.
  std::optional<double> value;
};

/**
 * Stores aggregated statistics for a measurement series.
 */
struct Stats {
  /// Whether statistics were successfully computed for a non-empty series.
  bool ok = false;
  /// Smallest valid value in the series.
  double min = 0.0;
  /// Largest valid value in the series.
  double max = 0.0;
  /// Average computed only from valid values.
  double avg = 0.0;
  /// Timestamp of the minimum value.
  QDateTime minAt;
  /// Timestamp of the maximum value.
  QDateTime maxAt;
  /// Number of valid points used in the calculations.
  int count = 0;
  /// Number of points without a value.
  int missing = 0;
  /// Simplified trend: up, down, flat, or unknown.
  QString trend; // up | down | flat | unknown
};
