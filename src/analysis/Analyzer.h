#pragma once
#include "../model/Types.h"

/**
 * Computes basic statistics for a measurement series.
 * This class is stateless and purely computational.
 */
struct SeriesCoverage {
  /// Whether a valid time span could be derived.
  bool ok = false;
  /// Number of points with a valid timestamp.
  int pointCount = 0;
  /// Earliest timestamp in the series.
  QDateTime firstAt;
  /// Latest timestamp in the series.
  QDateTime lastAt;
  /// Span of the series expressed in full hours.
  qint64 spanHours = 0;
  /// Span of the series expressed in days as a floating-point value.
  double spanDays = 0.0;
};

class Analyzer {
public:
  /**
   * Computes the minimum, maximum, average, and trend for the provided points.
   * Missing values are skipped in calculations but still counted.
   */
  static Stats compute(const QVector<MeasurementPoint>& pts);
  /**
   * Computes the actual time coverage of the series so the UI can show
   * how much online or local data is really available.
   */
  static SeriesCoverage computeCoverage(const QVector<MeasurementPoint>& pts);
};
