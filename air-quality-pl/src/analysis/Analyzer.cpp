#include "Analyzer.h"
#include <limits>
#include <algorithm>

static QString trendFrom(double firstAvg, double lastAvg) {
  const double diff = lastAvg - firstAvg;
  if (diff > 0.5) return "up";
  if (diff < -0.5) return "down";
  return "flat";
}

Stats Analyzer::compute(const QVector<MeasurementPoint>& pts) {
  Stats s;
  if (pts.isEmpty()) return s;

  double sum = 0.0;
  int n = 0;
  int missing = 0;

  double mn = std::numeric_limits<double>::infinity();
  double mx = -std::numeric_limits<double>::infinity();
  QDateTime mnAt, mxAt;

  QVector<double> values;
  values.reserve(pts.size());

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

  s.ok = true;
  s.min = mn; s.max = mx;
  s.avg = sum / n;
  s.minAt = mnAt; s.maxAt = mxAt;
  s.count = n;
  s.missing = missing;

  const int k = values.size();
  const int chunk = std::max(1, k / 3);

  double firstSum = 0.0, lastSum = 0.0;
  for (int i = 0; i < chunk; i++) firstSum += values[i];
  for (int i = k - chunk; i < k; i++) lastSum += values[i];

  s.trend = trendFrom(firstSum / chunk, lastSum / chunk);
  return s;
}