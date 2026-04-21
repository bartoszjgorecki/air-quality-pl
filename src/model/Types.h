#pragma once

#include <QString>
#include <QDateTime>
#include <QVector>
#include <optional>

/**
 * Reprezentuje pojedynczy punkt pomiarowy w czasie.
 */
struct MeasurementPoint {
  /// Znacznik czasu pojedynczego pomiaru.
  QDateTime dt;
  /// Wartość pomiaru; `std::nullopt` oznacza brak danych w źródle.
  std::optional<double> value;
};

/**
 * Przechowuje zagregowane statystyki dla serii pomiarowej.
 */
struct Stats {
  /// Informuje, czy statystyki udało się policzyć dla niepustej serii.
  bool ok = false;
  /// Najmniejsza poprawna wartość w serii.
  double min = 0.0;
  /// Największa poprawna wartość w serii.
  double max = 0.0;
  /// Średnia policzona wyłącznie z poprawnych wartości.
  double avg = 0.0;
  /// Czas wystąpienia wartości minimalnej.
  QDateTime minAt;
  /// Czas wystąpienia wartości maksymalnej.
  QDateTime maxAt;
  /// Liczba poprawnych punktów użytych w obliczeniach.
  int count = 0;
  /// Liczba punktów bez wartości.
  int missing = 0;
  /// Uproszczony trend: wzrost, spadek, stabilnie albo nieznany.
  QString trend; // up | down | flat | unknown
};
