#pragma once
#include "../model/Types.h"

/**
 * Computes aggregate statistics for measurement series.
 */
class Analyzer {
public:
  /**
   * Computes min, max, average and trend for the given points.
   */
  static Stats compute(const QVector<MeasurementPoint>& pts);
};
