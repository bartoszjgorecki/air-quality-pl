#include "AppController.h"
#include "../analysis/Analyzer.h"

#include <QDir>
#include <QSet>
#include <algorithm>
#include <exception>
#include <memory>

AppController::AppController(QObject* parent)
  : QObject(parent),
    m_db(QDir::current().filePath("data/db.json")),
    m_gios(this) {

  QString err;
  if (!m_db.ensureExists(&err)) {
    setOffline(true);
    setBanner("DB error: " + err);
  } else {
    setOffline(true);
    setBanner("Offline: local database ready");
  }
  refreshHistoryAvailability();
  setMapOverlayStatus("Select a sensor to color the map.");

  connect(&m_gios, &GiosClient::stationsReady, this, [this](QVariantList s) {
    try {
      m_stations = std::move(s);
      emit stationsChanged();
      setOffline(false);
      setBanner("Online: stations loaded");
    } catch (const std::exception& ex) {
      setOffline(true);
      setBanner("Offline: station processing error: " + QString::fromUtf8(ex.what()));
    } catch (...) {
      setOffline(true);
      setBanner("Offline: unknown station processing error");
    }
  });

  connect(&m_gios, &GiosClient::sensorsReady, this, [this](QVariantList s) {
    try {
      m_sensors = std::move(s);
      emit sensorsChanged();
      setOffline(false);
      setBanner("Online: sensors loaded");
    } catch (const std::exception& ex) {
      setOffline(true);
      setBanner("Offline: sensor processing error: " + QString::fromUtf8(ex.what()));
    } catch (...) {
      setOffline(true);
      setBanner("Offline: unknown sensor processing error");
    }
  });

  connect(&m_gios, &GiosClient::measurementsReady, this, [this](int sensorId, QString paramCode, QVariantList points) {
    try {
      setCurrentSensorId(sensorId);
      const QString selectedParamCode = sensorParamCode(sensorId);
      if (!selectedParamCode.isEmpty()) {
        setCurrentParamCode(selectedParamCode);
      } else if (m_currentParamCode.isEmpty()) {
        setCurrentParamCode(std::move(paramCode));
      }

      m_chartPoints = points;
      emit chartPointsChanged();

      // Build the C++ domain series used by the analyzer and local storage.
      m_currentSeries.clear();
      m_currentSeries.reserve(points.size());
      for (const auto& v : points) {
        const auto m = v.toMap();
        MeasurementPoint p;
        p.dt = QDateTime::fromMSecsSinceEpoch(m.value("t").toLongLong());
        p.value = m.value("v").toDouble();
        m_currentSeries.push_back(p);
      }
      setStatsFromPoints(m_currentSeries);
      refreshHistoryAvailability();

      setOffline(false);
      setBanner("Online: measurements loaded (" + describeParamCode(m_currentParamCode) + ")");
    } catch (const std::exception& ex) {
      setOffline(true);
      setBanner("Offline: measurement processing error: " + QString::fromUtf8(ex.what()));
    } catch (...) {
      setOffline(true);
      setBanner("Offline: unknown measurement processing error");
    }
  });

  connect(&m_gios, &GiosClient::error, this, [this](const QString& msg) {
    setOffline(true);
    refreshHistoryAvailability();
    if (m_historyAvailable) {
      setBanner("Offline: " + msg + ". Local history is available.");
    } else {
      setBanner("Offline: " + msg);
    }
  });
}

void AppController::setBanner(QString b) {
  if (m_banner == b) return;
  m_banner = std::move(b);
  emit bannerChanged();
}

void AppController::setOffline(bool v) {
  if (m_offline == v) return;
  m_offline = v;
  emit offlineChanged();
}

void AppController::setHistoryAvailable(bool v) {
  if (m_historyAvailable == v) return;
  m_historyAvailable = v;
  emit historyAvailableChanged();
}

void AppController::setCurrentSensorId(int sensorId) {
  if (m_currentSensorId == sensorId) return;
  m_currentSensorId = sensorId;
  emit currentSensorIdChanged();
  refreshHistoryAvailability();
}

void AppController::setCurrentParamCode(QString paramCode) {
  if (m_currentParamCode == paramCode) return;
  m_currentParamCode = std::move(paramCode);
  emit currentParamCodeChanged();
}

QString AppController::sensorParamCode(int sensorId) const {
  if (sensorId <= 0) return {};

  for (const auto& sensorValue : m_sensors) {
    const auto sensor = sensorValue.toMap();
    if (sensor.value("id").toInt() != sensorId) continue;
    return sensor.value("paramCode").toString().trimmed();
  }

  return {};
}

QString AppController::describeParamCode(const QString& paramCode) const {
  const QString normalizedCode = paramCode.trimmed();
  if (normalizedCode.isEmpty()) return {};

  for (const auto& sensorValue : m_sensors) {
    const auto sensor = sensorValue.toMap();
    const QString sensorCode = sensor.value("paramCode").toString().trimmed();
    if (sensorCode.compare(normalizedCode, Qt::CaseInsensitive) != 0) continue;

    const QString paramName = sensor.value("displayName").toString().trimmed().isEmpty()
      ? sensor.value("paramName").toString().trimmed()
      : sensor.value("displayName").toString().trimmed();
    if (!paramName.isEmpty() && !sensorCode.isEmpty()) {
      return paramName + " (" + sensorCode + ")";
    }
    if (!sensorCode.isEmpty()) {
      return sensorCode;
    }
  }

  return normalizedCode;
}

void AppController::setMapOverlayStatus(QString status) {
  if (m_mapOverlayStatus == status) return;
  m_mapOverlayStatus = std::move(status);
  emit mapOverlayStatusChanged();
}

void AppController::updateMapMetricRange() {
  QVariantMap nextRange;
  bool hasValue = false;
  double minValue = 0.0;
  double maxValue = 0.0;
  int count = 0;

  for (auto it = m_mapStationMetrics.cbegin(); it != m_mapStationMetrics.cend(); ++it) {
    const auto metric = it.value().toMap();
    if (metric.value("status").toString() != "ok") continue;

    const double value = metric.value("value").toDouble();
    if (!hasValue) {
      minValue = value;
      maxValue = value;
      hasValue = true;
    } else {
      minValue = std::min(minValue, value);
      maxValue = std::max(maxValue, value);
    }
    count++;
  }

  if (hasValue) {
    nextRange["min"] = minValue;
    nextRange["max"] = maxValue;
    nextRange["count"] = count;
    nextRange["paramCode"] = m_currentParamCode;
    nextRange["paramLabel"] = describeParamCode(m_currentParamCode);
  }

  if (m_mapMetricRange == nextRange) return;
  m_mapMetricRange = std::move(nextRange);
  emit mapMetricRangeChanged();
}

void AppController::setMapStationMetric(int stationId, QVariantMap metric) {
  const QString key = QString::number(stationId);
  if (m_mapStationMetrics.value(key).toMap() == metric) return;
  m_mapStationMetrics.insert(key, metric);
  emit mapStationMetricsChanged();
  updateMapMetricRange();
}

void AppController::refreshHistoryAvailability() {
  setHistoryAvailable(m_currentSensorId >= 0 && m_db.hasAnySeries(m_currentSensorId));
}

void AppController::setStatsFromPoints(const QVector<MeasurementPoint>& pts) {
  const Stats s = Analyzer::compute(pts);

  QVariantMap m;
  m["ok"] = s.ok;
  m["min"] = s.min;
  m["max"] = s.max;
  m["avg"] = s.avg;
  m["minAt"] = s.minAt.toString(Qt::ISODate);
  m["maxAt"] = s.maxAt.toString(Qt::ISODate);
  m["count"] = s.count;
  m["missing"] = s.missing;
  m["trend"] = s.trend;

  m_stats = std::move(m);
  emit statsChanged();
}

void AppController::refreshStations() {
  try {
    setBanner("Online: downloading stations...");
    m_gios.fetchStations();
  } catch (const std::exception& ex) {
    setOffline(true);
    setBanner("Offline: exception while loading stations: " + QString::fromUtf8(ex.what()));
  } catch (...) {
    setOffline(true);
    setBanner("Offline: unknown exception while loading stations");
  }
}

void AppController::loadSensors(int stationId) {
  try {
    m_sensors.clear();
    emit sensorsChanged();
    setCurrentSensorId(-1);
    setCurrentParamCode({});
    clearMapMeasurements();
    setBanner("Online: downloading sensors for station " + QString::number(stationId) + "...");
    m_gios.fetchSensors(stationId);
  } catch (const std::exception& ex) {
    setOffline(true);
    setBanner("Offline: exception while loading sensors: " + QString::fromUtf8(ex.what()));
  } catch (...) {
    setOffline(true);
    setBanner("Offline: unknown exception while loading sensors");
  }
}

void AppController::loadOnline(int sensorId) {
  try {
    setCurrentSensorId(sensorId);
    const QString selectedParamCode = sensorParamCode(sensorId);
    if (!selectedParamCode.isEmpty()) {
      setCurrentParamCode(selectedParamCode);
    }
    const QString sensorLabel = !selectedParamCode.isEmpty()
      ? describeParamCode(selectedParamCode)
      : ("sensor " + QString::number(sensorId));
    setBanner("Online: downloading measurements for " + sensorLabel + "...");
    m_gios.fetchMeasurements(sensorId);
  } catch (const std::exception& ex) {
    setOffline(true);
    setBanner("Offline: exception while loading measurements: " + QString::fromUtf8(ex.what()));
  } catch (...) {
    setOffline(true);
    setBanner("Offline: unknown exception while loading measurements");
  }
}

void AppController::saveCurrentToDb() {
  if (m_currentSensorId < 0 || m_currentSeries.isEmpty()) {
    setBanner("DB: no data to save");
    return;
  }
  try {
    QString err;
    if (!m_db.upsertSeries(m_currentSensorId, m_currentParamCode, m_currentSeries, &err)) {
      setOffline(true);
      setBanner("DB error: " + err);
      return;
    }
    refreshHistoryAvailability();
    setBanner("DB: series saved (sensorId=" + QString::number(m_currentSensorId) + ")");
  } catch (const std::exception& ex) {
    setOffline(true);
    setBanner("DB error: exception while saving: " + QString::fromUtf8(ex.what()));
  } catch (...) {
    setOffline(true);
    setBanner("DB error: unknown exception while saving");
  }
}

void AppController::loadHistory(int sensorId, const QString& fromIso, const QString& toIso) {
  try {
    const auto from = QDateTime::fromString(fromIso, Qt::ISODate);
    const auto to = QDateTime::fromString(toIso, Qt::ISODate);
    if (!from.isValid() || !to.isValid() || from > to) {
      setOffline(true);
      setBanner("DB error: invalid date range");
      return;
    }

    QString err;
    auto pts = m_db.loadHistory(sensorId, from, to, &err);
    if (!err.isEmpty()) {
      setOffline(true);
      setBanner("DB error: " + err);
      return;
    }
    if (pts.isEmpty()) {
      setCurrentSensorId(sensorId);
      setOffline(true);
      refreshHistoryAvailability();
      setBanner("DB: no local history for sensorId=" + QString::number(sensorId) + " in the selected range");
      return;
    }

    QVariantList list;
    list.reserve(pts.size());
    for (const auto& p : pts) {
      if (!p.value.has_value()) continue;
      QVariantMap m;
      m["t"] = p.dt.toMSecsSinceEpoch();
      m["v"] = *p.value;
      list.push_back(m);
    }

    setCurrentSensorId(sensorId);
    m_chartPoints = std::move(list);
    emit chartPointsChanged();

    m_currentSeries = pts;
    setStatsFromPoints(pts);
    setOffline(true);
    setBanner("Offline: showing chart from local database");
  } catch (const std::exception& ex) {
    setOffline(true);
    setBanner("DB error: exception while reading: " + QString::fromUtf8(ex.what()));
  } catch (...) {
    setOffline(true);
    setBanner("DB error: unknown exception while reading");
  }
}

void AppController::loadCurrentHistory(int days) {
  if (m_currentSensorId < 0) {
    setOffline(true);
    setBanner("DB: select a sensor first");
    return;
  }
  if (days <= 0) {
    setOffline(true);
    setBanner("DB: number of days must be positive");
    return;
  }

  const auto to = QDateTime::currentDateTime();
  const auto from = to.addDays(-days);
  loadHistory(m_currentSensorId, from.toString(Qt::ISODate), to.toString(Qt::ISODate));
}

void AppController::clearMapMeasurements() {
  ++m_mapRequestToken;

  if (!m_mapStationMetrics.isEmpty()) {
    m_mapStationMetrics.clear();
    emit mapStationMetricsChanged();
  }
  if (!m_mapMetricRange.isEmpty()) {
    m_mapMetricRange.clear();
    emit mapMetricRangeChanged();
  }

  if (m_currentParamCode.isEmpty()) {
    setMapOverlayStatus("Select a sensor to color the map.");
  } else {
    setMapOverlayStatus("Map coloring cleared for " + describeParamCode(m_currentParamCode) + ".");
  }
}

void AppController::refreshMapMeasurements(const QVariantList& stationIds, const QString& paramCode) {
  ++m_mapRequestToken;
  const quint64 token = m_mapRequestToken;

  QString normalizedParam = paramCode.trimmed();
  if (normalizedParam.isEmpty()) {
    clearMapMeasurements();
    return;
  }
  const QString displayParam = describeParamCode(normalizedParam);

  QSet<int> uniqueIds;
  for (const auto& item : stationIds) {
    int stationId = 0;
    if (item.typeId() == QMetaType::QVariantMap) {
      stationId = item.toMap().value("id").toInt();
    } else {
      stationId = item.toInt();
    }
    if (stationId > 0) uniqueIds.insert(stationId);
  }

  if (uniqueIds.isEmpty()) {
    if (!m_mapStationMetrics.isEmpty()) {
      m_mapStationMetrics.clear();
      emit mapStationMetricsChanged();
    }
    if (!m_mapMetricRange.isEmpty()) {
      m_mapMetricRange.clear();
      emit mapMetricRangeChanged();
    }
    setMapOverlayStatus("No visible stations to color on the map.");
    return;
  }

  if (uniqueIds.size() > 40) {
    if (!m_mapStationMetrics.isEmpty()) {
      m_mapStationMetrics.clear();
      emit mapStationMetricsChanged();
    }
    if (!m_mapMetricRange.isEmpty()) {
      m_mapMetricRange.clear();
      emit mapMetricRangeChanged();
    }
    setMapOverlayStatus(
      "Refine filters to 40 or fewer visible stations to color the map for " + displayParam + "."
    );
    return;
  }

  m_mapStationMetrics.clear();
  for (int stationId : uniqueIds) {
    QVariantMap loadingMetric;
    loadingMetric["status"] = "loading";
    loadingMetric["paramCode"] = normalizedParam;
    m_mapStationMetrics[QString::number(stationId)] = loadingMetric;
  }
  emit mapStationMetricsChanged();
  if (!m_mapMetricRange.isEmpty()) {
    m_mapMetricRange.clear();
    emit mapMetricRangeChanged();
  }

  setMapOverlayStatus(
    "Loading latest " + displayParam + " values for " + QString::number(uniqueIds.size()) + " visible station(s)..."
  );

  struct MapBatchState {
    quint64 token = 0;
    QString paramCode;
    QString paramDisplay;
    int total = 0;
    int completed = 0;
    int success = 0;
    int missing = 0;
    int failed = 0;
  };

  auto state = std::make_shared<MapBatchState>();
  state->token = token;
  state->paramCode = normalizedParam;
  state->paramDisplay = displayParam;
  state->total = uniqueIds.size();

  auto finishStation = [this, state](int stationId, QVariantMap metric) {
    if (state->token != m_mapRequestToken) return;

    setMapStationMetric(stationId, metric);

    const QString status = metric.value("status").toString();
    if (status == "ok") state->success++;
    else if (status == "missing") state->missing++;
    else state->failed++;

    state->completed++;
    if (state->completed < state->total) return;

    if (state->success > 0) {
      setMapOverlayStatus(
        "Map colors show the latest " + state->paramDisplay + " values for "
        + QString::number(state->success) + "/" + QString::number(state->total) + " station(s)."
      );
    } else if (state->missing == state->total) {
      setMapOverlayStatus(
        "No visible stations provide " + state->paramDisplay + " measurements."
      );
    } else {
      setMapOverlayStatus(
        "Map coloring for " + state->paramDisplay + " finished with partial data."
      );
    }
  };

  for (int stationId : uniqueIds) {
    m_gios.fetchSensorsForStation(
      stationId,
      [this, state, stationId, finishStation](int, QVariantList sensors) {
        if (state->token != m_mapRequestToken) return;

        int matchingSensorId = -1;
        QString matchingParamCode = state->paramCode;
        for (const auto& sensorValue : sensors) {
          const auto sensor = sensorValue.toMap();
          const QString sensorParam = sensor.value("paramCode").toString();
          if (sensorParam.compare(state->paramCode, Qt::CaseInsensitive) != 0) continue;
          matchingSensorId = sensor.value("id").toInt();
          matchingParamCode = sensorParam;
          break;
        }

        if (matchingSensorId < 0) {
          QVariantMap metric;
          metric["status"] = "missing";
          metric["paramCode"] = state->paramCode;
          finishStation(stationId, std::move(metric));
          return;
        }

        m_gios.fetchMeasurementsForSensor(
          matchingSensorId,
          [this, state, stationId, matchingParamCode, finishStation](int, QString returnedParamCode, QVariantList points) {
            if (state->token != m_mapRequestToken) return;

            if (points.isEmpty()) {
              QVariantMap metric;
              metric["status"] = "missing";
              metric["paramCode"] = !returnedParamCode.isEmpty() ? returnedParamCode : matchingParamCode;
              finishStation(stationId, std::move(metric));
              return;
            }

            const auto latestPoint = points.last().toMap();
            QVariantMap metric;
            metric["status"] = "ok";
            metric["paramCode"] = !returnedParamCode.isEmpty() ? returnedParamCode : matchingParamCode;
            metric["value"] = latestPoint.value("v").toDouble();
            metric["time"] = latestPoint.value("t").toLongLong();
            finishStation(stationId, std::move(metric));
          },
          [this, state, stationId, finishStation](QString message) {
            if (state->token != m_mapRequestToken) return;
            QVariantMap metric;
            metric["status"] = "error";
            metric["message"] = std::move(message);
            finishStation(stationId, std::move(metric));
          }
        );
      },
      [this, state, stationId, finishStation](QString message) {
        if (state->token != m_mapRequestToken) return;
        QVariantMap metric;
        metric["status"] = "error";
        metric["message"] = std::move(message);
        finishStation(stationId, std::move(metric));
      }
    );
  }
}
