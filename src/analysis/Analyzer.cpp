#include "Analyzer.h"
#include <limits>
#include <algorithm>

// Pomocnicza funkcja wyznacza prosty trend na podstawie dwóch średnich.
static QString trendFrom(double firstAvg, double lastAvg) {
  // Trend liczony jest bardzo prosto: porównujemy średni początek i koniec serii.
  const double diff = lastAvg - firstAvg;
  if (diff > 0.5) return "up";
  if (diff < -0.5) return "down";
  return "flat";
}

// Pokrycie czasowe serii liczymy osobno od statystyk, bo użytkownik może
// zażądać większego zakresu dni niż rzeczywiście udostępnia API.
SeriesCoverage Analyzer::computeCoverage(const QVector<MeasurementPoint>& pts) {
  SeriesCoverage coverage;

  for (const auto& p : pts) {
    if (!p.dt.isValid()) continue;

    if (!coverage.firstAt.isValid() || p.dt < coverage.firstAt) {
      coverage.firstAt = p.dt;
    }
    if (!coverage.lastAt.isValid() || p.dt > coverage.lastAt) {
      coverage.lastAt = p.dt;
    }
    coverage.pointCount++;
  }

  if (coverage.pointCount == 0 || !coverage.firstAt.isValid() || !coverage.lastAt.isValid()) {
    return coverage;
  }

  const qint64 spanMs = std::max<qint64>(0, coverage.firstAt.msecsTo(coverage.lastAt));
  coverage.ok = true;
  coverage.spanHours = spanMs / (1000LL * 60LL * 60LL);
  coverage.spanDays = static_cast<double>(spanMs) / (1000.0 * 60.0 * 60.0 * 24.0);
  return coverage;
}

// Główna funkcja analizatora zwraca gotowy pakiet statystyk dla serii.
Stats Analyzer::compute(const QVector<MeasurementPoint>& pts) {
  Stats s;
  // Pusta seria nie daje sensownych statystyk, więc zostawiamy domyślny wynik.
  if (pts.isEmpty()) return s;

  double sum = 0.0;
  int n = 0;
  int missing = 0;

  double mn = std::numeric_limits<double>::infinity();
  double mx = -std::numeric_limits<double>::infinity();
  QDateTime mnAt, mxAt;

  // Najpierw filtrujemy brakujące wartości i zbieramy dane do dalszych obliczeń.
  QVector<double> values;
  values.reserve(pts.size());

  // Jednocześnie filtrujemy braki i liczymy min, max oraz sumę do średniej.
  for (const auto& p : pts) {
    if (!p.value.has_value()) { missing++; continue; }
    const double v = *p.value;
    values.push_back(v);
    sum += v;
    n++;
    if (v < mn) { mn = v; mnAt = p.dt; }
    if (v > mx) { mx = v; mxAt = p.dt; }
  }

  if (n == 0) return s;

  // Po zebraniu wszystkich poprawnych wartości wypełniamy wynik końcowy.
  s.ok = true;
  s.min = mn; s.max = mx;
  s.avg = sum / n;
  s.minAt = mnAt; s.maxAt = mxAt;
  s.count = n;
  s.missing = missing;

  // Trend bazuje na porównaniu pierwszej i ostatniej części serii.
  const int k = values.size();
  // Dzielimy serię orientacyjnie na trzy części i porównujemy początek z końcem.
  const int chunk = std::max(1, k / 3);

  double firstSum = 0.0, lastSum = 0.0;
  for (int i = 0; i < chunk; i++) firstSum += values[i];
  for (int i = k - chunk; i < k; i++) lastSum += values[i];

  s.trend = trendFrom(firstSum / chunk, lastSum / chunk);
  return s;
}
