#pragma once
#include "../model/Types.h"

/**
 * Liczy podstawowe statystyki dla serii pomiarowej.
 */
class Analyzer {
public:
  /**
   * Oblicza minimum, maksimum, średnią i trend dla przekazanych punktów.
   */
  static Stats compute(const QVector<MeasurementPoint>& pts);
};
