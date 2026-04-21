#pragma once
#include "../model/Types.h"

/**
 * Oblicza podstawowe statystyki dla serii pomiarowej.
 * Ta klasa nie przechowuje stanu i pełni wyłącznie rolę obliczeniową.
 */
struct SeriesCoverage {
  /// Informuje, czy udało się wyznaczyć poprawny zakres czasu.
  bool ok = false;
  /// Liczba punktów z poprawnym znacznikiem czasu.
  int pointCount = 0;
  /// Najwcześniejszy czas występujący w serii.
  QDateTime firstAt;
  /// Najpóźniejszy czas występujący w serii.
  QDateTime lastAt;
  /// Rozpiętość serii wyrażona w pełnych godzinach.
  qint64 spanHours = 0;
  /// Rozpiętość serii wyrażona w dniach jako liczba zmiennoprzecinkowa.
  double spanDays = 0.0;
};

class Analyzer {
public:
  /**
   * Oblicza minimum, maksimum, średnią i trend dla podanych punktów.
   * Brakujące wartości są pomijane w obliczeniach, ale nadal zliczane.
   */
  static Stats compute(const QVector<MeasurementPoint>& pts);
  /**
   * Oblicza rzeczywiste pokrycie czasowe serii, aby interfejs mógł pokazać,
   * ile danych online albo lokalnych jest faktycznie dostępnych.
   */
  static SeriesCoverage computeCoverage(const QVector<MeasurementPoint>& pts);
};
