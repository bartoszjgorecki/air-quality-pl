#pragma once

#include <QString>
#include <QDateTime>
#include <QVector>
#include <optional>

/**
 * Represents a single measurement point in time.
 */
struct MeasurementPoint {
  QDateTime dt;
  std::optional<double> value;
};

/**
 * Holds computed aggregate statistics for a measurement series.
 */
struct Stats {
  bool ok = false;
  double min = 0.0;
  double max = 0.0;
  double avg = 0.0;
  QDateTime minAt;
  QDateTime maxAt;
  int count = 0;
  int missing = 0;
  QString trend; // up | down | flat | unknown
};
