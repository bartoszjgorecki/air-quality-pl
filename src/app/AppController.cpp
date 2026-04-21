#include "AppController.h"
#include "../analysis/Analyzer.h"

#include <QDir>
#include <QSet>
#include <algorithm>
#include <cmath>
#include <exception>
#include <memory>

namespace {

// Buduje czytelny opis długości serii, który potem pokazujemy użytkownikowi w UI.
QString formatCoverageSpan(const SeriesCoverage& coverage) {
  if (!coverage.ok) return {};

  if (coverage.pointCount <= 1 || coverage.spanHours <= 0) {
    return coverage.pointCount == 1
      ? QString("1 measurement point")
      : QString::number(coverage.pointCount) + " measurement point(s)";
  }

  const int decimals = coverage.spanDays < 10.0 ? 1 : 0;
  return QString("%1 hour(s) (~%2 day(s))")
    .arg(coverage.spanHours)
    .arg(QString::number(coverage.spanDays, 'f', decimals));
}

// Zaokrąglenie w górę pozwala łatwo porównać żądany zakres dni z tym,
// ile danych faktycznie udało się pobrać.
int roundedCoverageDays(const SeriesCoverage& coverage) {
  if (!coverage.ok) return 0;
  if (coverage.pointCount <= 1 || coverage.spanDays <= 0.0) return 1;
  return std::max(1, static_cast<int>(std::ceil(coverage.spanDays)));
}

// Ten tekst wyjaśnia, dlaczego większa liczba dni czasem nie zmienia wykresu.
QString buildChartRangeInfoText(const SeriesCoverage& coverage, int requestedDays, bool localHistory) {
  if (!coverage.ok) {
    return localHistory
      ? QString("The loaded local history does not contain valid timestamps yet.")
      : QString("The current online series does not contain valid timestamps yet.");
  }

  const QString coverageText = formatCoverageSpan(coverage);
  const int availableDays = roundedCoverageDays(coverage);

  if (localHistory) {
    if (requestedDays > availableDays) {
      return "Local history currently covers only " + coverageText
        + ", so larger chart ranges will look the same until more data is saved in the JSON database.";
    }
    return "Local history currently loaded on the chart covers " + coverageText + ".";
  }

  if (requestedDays > availableDays) {
    return "The GIOS online API currently returned only " + coverageText
      + " for this sensor, so larger chart ranges will look the same. "
      + "Save the series to the local JSON database and switch to OFFLINE view for longer local comparisons.";
  }

  return "Online data currently covers " + coverageText + ".";
}

// Baner w nagłówku ma być krótki, ale ma od razu tłumaczyć ograniczenie API.
QString buildOnlineMeasurementBanner(const QString& sensorLabel, const SeriesCoverage& coverage, int requestedDays) {
  if (!coverage.ok) {
    return "Online: measurements loaded for " + sensorLabel;
  }

  const int availableDays = roundedCoverageDays(coverage);
  if (requestedDays > availableDays) {
    return "Online: showing all available data for " + sensorLabel
      + " (GIOS returned only " + formatCoverageSpan(coverage) + ")";
  }

  return "Online: showing last " + QString::number(requestedDays)
    + " day(s) for " + sensorLabel;
}

}  // namespace

// Konstruktor przygotowuje stan startowy aplikacji i spina sygnały klienta GIOS
// z właściwościami, które potem obserwuje interfejs QML.
AppController::AppController(QObject* parent)
  : QObject(parent),
    m_db(QDir::current().filePath("data/db.json")),
    m_gios(this) {

  // Przy starcie dbamy o to, żeby lokalna baza istniała nawet przed pierwszym pobraniem online.
  QString err;
  if (!m_db.ensureExists(&err)) {
    setOffline(true);
    setBanner("DB error: " + err);
  } else {
    setOffline(true);
    setBanner("Offline: local database ready");

    // Czyścimy stare wpisy stacji, które kiedyś mogły zostać zapisane zbiorczo
    // z API, ale nie mają żadnych sensorów i nie są użyteczne offline.
    QString pruneErr;
    if (!m_db.pruneStationsWithoutCachedSensors(&pruneErr)) {
      setBanner("Offline: local database ready, but offline station cleanup failed: " + pruneErr);
    }

    // Jeśli mamy już zapisane stacje offline, pokazujemy je od razu po starcie,
    // nawet zanim użytkownik kliknie przycisk pobierania.
    QString cachedStationsErr;
    m_offlineStations = m_db.loadOfflineStations(&cachedStationsErr);
    if (cachedStationsErr.isEmpty() && !m_offlineStations.isEmpty()) {
      setBanner("Offline: loaded " + QString::number(m_offlineStations.size())
                + " cached station(s) from the local database at startup.");
    } else if (!cachedStationsErr.isEmpty()) {
      setBanner("Offline: local database ready, but cached stations could not be read: " + cachedStationsErr);
    }
  }
  refreshHistoryAvailability();
  setMapOverlayStatus("Select a sensor to color the map.");
  setChartRangeInfo("Select a sensor to load online measurements or local history.");

  // Te połączenia sprawiają, że AppController reaguje na gotowe dane z klienta GIOS
  // i zamienia je na stan bezpośrednio używany przez interfejs.
  connect(&m_gios, &GiosClient::stationsReady, this, [this](QVariantList s) {
    try {
      m_onlineStations = std::move(s);
      setStationViewOffline(false);
      emit stationsChanged();
      setOffline(false);
      m_pendingRequest = PendingRequest::None;
      m_pendingStationId = -1;
      setBanner("Online: stations loaded");
    } catch (const std::exception& ex) {
      m_pendingRequest = PendingRequest::None;
      m_pendingStationId = -1;
      setOffline(true);
      setBanner("Offline: station processing error: " + QString::fromUtf8(ex.what()));
    } catch (...) {
      m_pendingRequest = PendingRequest::None;
      m_pendingStationId = -1;
      setOffline(true);
      setBanner("Offline: unknown station processing error");
    }
  });

  connect(&m_gios, &GiosClient::sensorsReady, this, [this](QVariantList s) {
    try {
      m_sensors = std::move(s);
      emit sensorsChanged();
      setOffline(false);
      const int stationId = m_pendingStationId > 0 ? m_pendingStationId : m_currentStationId;
      m_pendingRequest = PendingRequest::None;
      m_pendingStationId = -1;

      QString cacheErr;
      bool cacheOk = true;

      if (stationId > 0) {
        const QVariantMap stationToStore = stationById(stationId);

        if (!stationToStore.isEmpty() && !m_db.upsertStation(stationToStore, &cacheErr)) {
          cacheOk = false;
        } else if (!m_db.upsertSensorsForStation(stationId, m_sensors, &cacheErr)) {
          cacheOk = false;
        }

        if (cacheOk) {
          refreshOfflineStations();
        }
      }

      if (!cacheOk) {
        setBanner("Online: sensors loaded, but offline cache update failed: " + cacheErr);
      } else {
        setBanner("Online: sensors loaded");
      }
    } catch (const std::exception& ex) {
      m_pendingRequest = PendingRequest::None;
      m_pendingStationId = -1;
      setOffline(true);
      setBanner("Offline: sensor processing error: " + QString::fromUtf8(ex.what()));
    } catch (...) {
      m_pendingRequest = PendingRequest::None;
      m_pendingStationId = -1;
      setOffline(true);
      setBanner("Offline: unknown sensor processing error");
    }
  });

  connect(&m_gios, &GiosClient::measurementsReady, this, [this](int sensorId, QString paramCode, QVariantList points) {
    try {
      m_pendingRequest = PendingRequest::None;
      setCurrentSensorId(sensorId);
      const QString selectedParamCode = sensorParamCode(sensorId);
      if (!selectedParamCode.isEmpty()) {
        setCurrentParamCode(selectedParamCode);
      } else if (m_currentParamCode.isEmpty()) {
        setCurrentParamCode(std::move(paramCode));
      }

      // Zachowujemy pełną serię źródłową, a dopiero później wycinamy z niej
      // zakres dni wybrany przez użytkownika dla wykresu.
      m_sourceSeries.clear();
      m_sourceSeries.reserve(points.size());
      for (const auto& v : points) {
        const auto m = v.toMap();
        MeasurementPoint p;
        p.dt = QDateTime::fromMSecsSinceEpoch(m.value("t").toLongLong());
        p.value = m.value("v").toDouble();
        m_sourceSeries.push_back(p);
      }
      m_showingLocalHistory = false;
      updateDisplayedSeries(m_currentChartRangeDays);
      refreshHistoryAvailability();

      setOffline(false);
      const SeriesCoverage coverage = Analyzer::computeCoverage(m_sourceSeries);
      setBanner(buildOnlineMeasurementBanner(describeParamCode(m_currentParamCode), coverage, m_currentChartRangeDays));
    } catch (const std::exception& ex) {
      m_pendingRequest = PendingRequest::None;
      setOffline(true);
      setBanner("Offline: measurement processing error: " + QString::fromUtf8(ex.what()));
    } catch (...) {
      m_pendingRequest = PendingRequest::None;
      setOffline(true);
      setBanner("Offline: unknown measurement processing error");
    }
  });

  connect(&m_gios, &GiosClient::error, this, [this](const QString& msg) {
    const PendingRequest failedRequest = m_pendingRequest;
    const int failedStationId = m_pendingStationId;
    m_pendingRequest = PendingRequest::None;
    m_pendingStationId = -1;

    if (failedRequest == PendingRequest::Stations && loadStationsFromLocalCache(msg)) {
      return;
    }
    if (failedRequest == PendingRequest::Sensors && loadSensorsFromLocalCache(failedStationId, msg)) {
      return;
    }

    setOffline(true);
    refreshHistoryAvailability();
    if (m_historyAvailable) {
      setBanner("Offline: " + msg + ". Local history is available.");
    } else {
      setBanner("Offline: " + msg);
    }
  });
}

// Ustawia główny komunikat statusowy widoczny w nagłówku aplikacji.
void AppController::setBanner(QString b) {
  if (m_banner == b) return;
  m_banner = std::move(b);
  emit bannerChanged();
}

// Ustawia pomocniczy opis zakresu danych widoczny przy sterowaniu wykresem.
void AppController::setChartRangeInfo(QString info) {
  if (m_chartRangeInfo == info) return;
  m_chartRangeInfo = std::move(info);
  emit chartRangeInfoChanged();
}

// Ustawia maksymalny zakres dni, jaki powinien być dostępny w kontrolce QML.
void AppController::setChartRangeMaxDays(int days) {
  const int normalizedDays = std::max(1, days);
  if (m_chartRangeMaxDays == normalizedDays) return;
  m_chartRangeMaxDays = normalizedDays;
  emit chartRangeMaxDaysChanged();
}

// Przełącza tryb online/offline używany przez banner i znacznik statusu.
void AppController::setOffline(bool v) {
  if (m_offline == v) return;
  m_offline = v;
  emit offlineChanged();
}

// Przełącza źródło listy stacji między lokalną bazą a pełną listą z API.
void AppController::setStationViewOffline(bool v) {
  if (m_stationViewOffline == v) return;
  m_stationViewOffline = v;
  emit stationViewModeChanged();
  emit stationsChanged();
}

// Zapamiętuje, czy dla bieżącego sensora mamy lokalną historię w JSON.
void AppController::setHistoryAvailable(bool v) {
  if (m_historyAvailable == v) return;
  m_historyAvailable = v;
  emit historyAvailableChanged();
}

// Aktualizuje identyfikator bieżącego sensora i przy okazji odświeża informację o historii.
void AppController::setCurrentSensorId(int sensorId) {
  if (m_currentSensorId == sensorId) return;
  m_currentSensorId = sensorId;
  emit currentSensorIdChanged();
  refreshHistoryAvailability();
}

// Odświeża lokalną listę stacji, które mają sensowny zestaw danych do pracy offline.
void AppController::refreshOfflineStations() {
  QString err;
  const QVariantList offlineStations = m_db.loadOfflineStations(&err);
  if (!err.isEmpty()) return;
  if (m_offlineStations == offlineStations) return;
  m_offlineStations = offlineStations;
  if (m_stationViewOffline) emit stationsChanged();
}

// Aktualizuje kod parametru, np. PM10 albo NO2, dla dalszej pracy wykresu i mapy.
void AppController::setCurrentParamCode(QString paramCode) {
  if (m_currentParamCode == paramCode) return;
  m_currentParamCode = std::move(paramCode);
  emit currentParamCodeChanged();
}

// Wyszukuje stację o podanym identyfikatorze najpierw w liście online, a potem lokalnej.
QVariantMap AppController::stationById(int stationId) const {
  if (stationId <= 0) return {};

  for (const auto& stationValue : m_onlineStations) {
    const auto station = stationValue.toMap();
    if (station.value("id").toInt() == stationId) return station;
  }

  for (const auto& stationValue : m_offlineStations) {
    const auto station = stationValue.toMap();
    if (station.value("id").toInt() == stationId) return station;
  }

  return {};
}

// Szuka w aktualnej liście sensorów kodu parametru dla podanego identyfikatora sensora.
QString AppController::sensorParamCode(int sensorId) const {
  if (sensorId <= 0) return {};

  for (const auto& sensorValue : m_sensors) {
    const auto sensor = sensorValue.toMap();
    if (sensor.value("id").toInt() != sensorId) continue;
    return sensor.value("paramCode").toString().trimmed();
  }

  return {};
}

// Buduje czytelną etykietę do UI, np. "Particulate matter PM10 (PM10)".
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

// Ustawia komunikat opisujący stan warstwy kolorowania mapy.
void AppController::setMapOverlayStatus(QString status) {
  if (m_mapOverlayStatus == status) return;
  m_mapOverlayStatus = std::move(status);
  emit mapOverlayStatusChanged();
}

// Liczy zakres min/max tylko z poprawnych wartości, żeby legenda mapy była wiarygodna.
void AppController::updateMapMetricRange() {
  QVariantMap nextRange;
  bool hasValue = false;
  double minValue = 0.0;
  double maxValue = 0.0;
  int count = 0;

  // Zakres jest liczony wyłącznie na podstawie poprawnie pobranych wartości.
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

// Zapisuje jeden wynik pomiarowy dla stacji widocznej na mapie.
void AppController::setMapStationMetric(int stationId, QVariantMap metric) {
  const QString key = QString::number(stationId);
  if (m_mapStationMetrics.value(key).toMap() == metric) return;
  m_mapStationMetrics.insert(key, metric);
  emit mapStationMetricsChanged();
  updateMapMetricRange();
}

// Sprawdza, czy aktualny sensor ma już jakąkolwiek serię zapisaną lokalnie.
void AppController::refreshHistoryAvailability() {
  setHistoryAvailable(m_currentSensorId >= 0 && m_db.hasAnySeries(m_currentSensorId));
}

// Przepisuje statystyki z modelu C++ do QVariantMap, z którego korzysta QML.
void AppController::setStatsFromPoints(const QVector<MeasurementPoint>& pts) {
  const Stats s = Analyzer::compute(pts);

  // QML najlepiej współpracuje z QVariantMap, więc tutaj robimy konwersję
  // z domenowej struktury Stats na obiekt czytelny dla interfejsu.
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

// Zamienia serię domenową na punkty wykresu udostępniane do QML.
void AppController::setChartFromPoints(const QVector<MeasurementPoint>& pts) {
  QVariantList list;
  list.reserve(pts.size());

  for (const auto& p : pts) {
    // Brakujących punktów nie rysujemy na wykresie liniowym.
    if (!p.value.has_value()) continue;

    QVariantMap point;
    point["t"] = p.dt.toMSecsSinceEpoch();
    point["v"] = *p.value;
    list.push_back(point);
  }

  m_chartPoints = std::move(list);
  emit chartPointsChanged();
}

// Zwraca końcowy wycinek serii odpowiadający ostatnim N dniom danych.
QVector<MeasurementPoint> AppController::filterSeriesToLastDays(const QVector<MeasurementPoint>& pts, int days) const {
  if (pts.isEmpty() || days <= 0) return pts;

  QDateTime latest;
  // Szukamy ostatniego poprawnego punktu, żeby zakres liczyć względem realnych danych.
  for (auto it = pts.crbegin(); it != pts.crend(); ++it) {
    if (!it->dt.isValid()) continue;
    latest = it->dt;
    break;
  }

  if (!latest.isValid()) return pts;

  const QDateTime from = latest.addDays(-days);
  QVector<MeasurementPoint> filtered;
  filtered.reserve(pts.size());

  for (const auto& p : pts) {
    if (!p.dt.isValid()) continue;
    if (p.dt < from || p.dt > latest) continue;
    filtered.push_back(p);
  }

  return filtered.isEmpty() ? pts : filtered;
}

// Aktualizuje serię widoczną na wykresie i odpowiadające jej statystyki.
void AppController::updateDisplayedSeries(int days) {
  // To jest jedno centralne miejsce, które odświeża zarówno wykres, jak i panel analizy.
  m_currentChartRangeDays = std::max(1, days);
  m_currentSeries = filterSeriesToLastDays(m_sourceSeries, m_currentChartRangeDays);
  setChartFromPoints(m_currentSeries);
  setStatsFromPoints(m_currentSeries);
  refreshChartRangeInfo();
  refreshChartRangeMaxDays();
}

// Buduje komunikat o tym, ile danych naprawdę obejmuje aktualna seria.
void AppController::refreshChartRangeInfo() {
  if (m_sourceSeries.isEmpty()) {
    setChartRangeInfo("Select a sensor to load online measurements or local history.");
    return;
  }

  const SeriesCoverage coverage = Analyzer::computeCoverage(m_sourceSeries);
  setChartRangeInfo(buildChartRangeInfoText(coverage, m_currentChartRangeDays, m_showingLocalHistory));
}

// Aktualizuje maksymalny wybieralny zakres dni na podstawie aktualnego źródła wykresu.
void AppController::refreshChartRangeMaxDays() {
  if (m_sourceSeries.isEmpty()) {
    setChartRangeMaxDays(30);
    return;
  }

  const SeriesCoverage coverage = Analyzer::computeCoverage(m_sourceSeries);
  const int maxDays = roundedCoverageDays(coverage);
  setChartRangeMaxDays(maxDays > 0 ? maxDays : 30);

  if (m_currentChartRangeDays > m_chartRangeMaxDays) {
    m_currentChartRangeDays = m_chartRangeMaxDays;
    m_currentSeries = filterSeriesToLastDays(m_sourceSeries, m_currentChartRangeDays);
    setChartFromPoints(m_currentSeries);
    setStatsFromPoints(m_currentSeries);
    refreshChartRangeInfo();
  }
}

// Próbuje podmienić listę stacji na ostatnią migawkę zapisaną lokalnie.
bool AppController::loadStationsFromLocalCache(const QString& failureReason) {
  QString err;
  const QVariantList cachedStations = m_db.loadOfflineStations(&err);
  if (!err.isEmpty()) {
    setOffline(true);
    setBanner("Offline: " + failureReason + ". Local station cache could not be read: " + err);
    return false;
  }
  if (cachedStations.isEmpty()) {
    return false;
  }

  m_offlineStations = cachedStations;
  setStationViewOffline(true);
  setOffline(true);
  setBanner("Offline: " + failureReason + ". Loaded " + QString::number(m_offlineStations.size())
            + " station(s) from the local database.");
  return true;
}

// Próbuje podmienić listę sensorów na dane zapisane wcześniej dla wskazanej stacji.
bool AppController::loadSensorsFromLocalCache(int stationId, const QString& failureReason) {
  if (stationId <= 0) return false;

  QString err;
  const QVariantList cachedSensors = m_db.loadSensorsForStation(stationId, &err);
  if (!err.isEmpty() && err != "no cached sensors for station") {
    setOffline(true);
    setBanner("Offline: " + failureReason + ". Local sensor cache could not be read: " + err);
    return false;
  }
  if (!err.isEmpty()) {
    return false;
  }

  err.clear();
  const QVariantList browsableSensors = m_db.loadSensorsWithSavedHistoryForStation(stationId, &err);
  if (!err.isEmpty()) {
    setOffline(true);
    setBanner("Offline: " + failureReason + ". Local sensor history could not be read: " + err);
    return false;
  }

  m_sensors = browsableSensors;
  emit sensorsChanged();
  setOffline(true);

  if (m_sensors.isEmpty()) {
    setBanner("Offline: " + failureReason + ". Cached sensors exist for station "
              + QString::number(stationId)
              + ", but none of them has saved local history yet.");
  } else {
    const QString filteredSuffix = m_sensors.size() < cachedSensors.size()
      ? (" with saved local history (" + QString::number(m_sensors.size())
         + "/" + QString::number(cachedSensors.size()) + ")")
      : "";
    setBanner("Offline: " + failureReason + ". Loaded " + QString::number(m_sensors.size())
              + " cached sensor(s) for station " + QString::number(stationId)
              + filteredSuffix + " from the local database.");
  }
  return true;
}

// Rozpoczyna pobieranie listy stacji z API.
void AppController::refreshStations() {
  try {
    m_pendingRequest = PendingRequest::Stations;
    m_pendingStationId = -1;
    setStationViewOffline(false);
    setBanner("Online: downloading stations...");
    m_gios.fetchStations();
  } catch (const std::exception& ex) {
    m_pendingRequest = PendingRequest::None;
    m_pendingStationId = -1;
    if (loadStationsFromLocalCache("exception while loading stations: " + QString::fromUtf8(ex.what()))) {
      return;
    }
    setOffline(true);
    setBanner("Offline: exception while loading stations: " + QString::fromUtf8(ex.what()));
  } catch (...) {
    m_pendingRequest = PendingRequest::None;
    m_pendingStationId = -1;
    if (loadStationsFromLocalCache("unknown exception while loading stations")) {
      return;
    }
    setOffline(true);
    setBanner("Offline: unknown exception while loading stations");
  }
}

// Przełącza badge źródła stacji między listą lokalną a listą online.
void AppController::toggleStationViewMode() {
  if (m_stationViewOffline) {
    if (m_onlineStations.isEmpty()) {
      refreshStations();
      return;
    }

    setStationViewOffline(false);
    setBanner("Online view: showing the downloaded station list from the API.");
  } else {
    refreshOfflineStations();
    setStationViewOffline(true);
    if (m_offlineStations.isEmpty()) {
      setBanner("Offline view: no locally cached stations are available yet.");
    } else {
      setBanner("Offline view: showing " + QString::number(m_offlineStations.size())
                + " locally cached station(s) from the JSON database.");
    }
  }

  m_currentStationId = -1;
  m_sensors.clear();
  emit sensorsChanged();
  setCurrentSensorId(-1);
  setCurrentParamCode({});
  clearMapMeasurements();
}

// Po wyborze stacji czyści poprzedni kontekst i pobiera nowe sensory.
void AppController::loadSensors(int stationId) {
  if (m_stationViewOffline) {
    m_currentStationId = stationId;
    m_pendingRequest = PendingRequest::None;
    m_pendingStationId = -1;
    m_sensors.clear();
    emit sensorsChanged();
    setCurrentSensorId(-1);
    setCurrentParamCode({});
    clearMapMeasurements();

    if (loadSensorsFromLocalCache(stationId, "using the local offline station view")) {
      return;
    }

    setOffline(true);
    setBanner("Offline: no cached sensors are available for station " + QString::number(stationId));
    return;
  }

  try {
    // Zmiana stacji oznacza wyczyszczenie poprzedniego wyboru sensora i warstwy mapy.
    m_currentStationId = stationId;
    m_pendingRequest = PendingRequest::Sensors;
    m_pendingStationId = stationId;
    m_sensors.clear();
    emit sensorsChanged();
    setCurrentSensorId(-1);
    setCurrentParamCode({});
    clearMapMeasurements();
    setBanner("Online: downloading sensors for station " + QString::number(stationId) + "...");
    m_gios.fetchSensors(stationId);
  } catch (const std::exception& ex) {
    m_pendingRequest = PendingRequest::None;
    m_pendingStationId = -1;
    if (loadSensorsFromLocalCache(stationId, "exception while loading sensors: " + QString::fromUtf8(ex.what()))) {
      return;
    }
    setOffline(true);
    setBanner("Offline: exception while loading sensors: " + QString::fromUtf8(ex.what()));
  } catch (...) {
    m_pendingRequest = PendingRequest::None;
    m_pendingStationId = -1;
    if (loadSensorsFromLocalCache(stationId, "unknown exception while loading sensors")) {
      return;
    }
    setOffline(true);
    setBanner("Offline: unknown exception while loading sensors");
  }
}

// Pobiera serię online dla wybranego sensora.
void AppController::loadOnline(int sensorId, int days) {
  try {
    m_pendingRequest = PendingRequest::Measurements;
    m_currentChartRangeDays = std::max(1, days);
    m_showingLocalHistory = false;
    setCurrentSensorId(sensorId);
    const QString selectedParamCode = sensorParamCode(sensorId);
    if (!selectedParamCode.isEmpty()) {
      setCurrentParamCode(selectedParamCode);
    }
    const QString sensorLabel = !selectedParamCode.isEmpty()
      ? describeParamCode(selectedParamCode)
      : ("sensor " + QString::number(sensorId));
    setBanner(
      "Online: downloading measurements for " + sensorLabel
      + " (last " + QString::number(m_currentChartRangeDays) + " day(s) on chart)..."
    );
    m_gios.fetchMeasurements(sensorId);
  } catch (const std::exception& ex) {
    m_pendingRequest = PendingRequest::None;
    setOffline(true);
    setBanner("Offline: exception while loading measurements: " + QString::fromUtf8(ex.what()));
  } catch (...) {
    m_pendingRequest = PendingRequest::None;
    setOffline(true);
    setBanner("Offline: unknown exception while loading measurements");
  }
}

// Ładuje serię sensora z lokalnej bazy albo z API zależnie od aktywnego widoku stacji.
void AppController::loadSensorData(int sensorId, int days) {
  const int normalizedDays = std::max(1, days);

  setCurrentSensorId(sensorId);
  const QString selectedParamCode = sensorParamCode(sensorId);
  if (!selectedParamCode.isEmpty()) {
    setCurrentParamCode(selectedParamCode);
  }

  if (m_stationViewOffline) {
    m_pendingRequest = PendingRequest::None;
    loadCurrentHistory(normalizedDays);
    return;
  }

  loadOnline(sensorId, normalizedDays);
}

// Zmienia zakres ostatnich N dni dla bieżącej serii bez ponownego pobierania.
void AppController::applyCurrentChartRange(int days) {
  if (days <= 0) return;
  m_currentChartRangeDays = days;
  // Jeśli jeszcze nic nie pobrano, samo ustawienie zakresu ma zostać zapamiętane.
  if (m_sourceSeries.isEmpty()) return;
  updateDisplayedSeries(days);
}

// Zapisuje ostatnio pobraną serię do lokalnej bazy JSON.
void AppController::saveCurrentToDb() {
  if (m_stationViewOffline || m_showingLocalHistory) {
    setBanner("DB: saving is available only for online measurements currently loaded from the API");
    return;
  }
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

// Wczytuje historię z lokalnej bazy dla sensora i zadanego zakresu dat.
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

    setCurrentSensorId(sensorId);
    m_sourceSeries = pts;
    m_showingLocalHistory = true;
    updateDisplayedSeries(m_currentChartRangeDays);
    setOffline(true);
    const SeriesCoverage coverage = Analyzer::computeCoverage(m_sourceSeries);
    if (coverage.ok) {
      setBanner("Offline: showing chart from local database (" + formatCoverageSpan(coverage) + ")");
    } else {
      setBanner("Offline: showing chart from local database");
    }
  } catch (const std::exception& ex) {
    setOffline(true);
    setBanner("DB error: exception while reading: " + QString::fromUtf8(ex.what()));
  } catch (...) {
    setOffline(true);
    setBanner("DB error: unknown exception while reading");
  }
}

// Wygodna nakładka na loadHistory: bierze ostatnie N dni dla bieżącego sensora.
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

  m_currentChartRangeDays = days;
  try {
    QString err;
    auto pts = m_db.loadAllHistory(m_currentSensorId, &err);
    if (!err.isEmpty()) {
      setOffline(true);
      setBanner("DB error: " + err);
      return;
    }
    if (pts.isEmpty()) {
      setOffline(true);
      refreshHistoryAvailability();
      setBanner("DB: no local history for sensorId=" + QString::number(m_currentSensorId));
      return;
    }

    m_sourceSeries = std::move(pts);
    m_showingLocalHistory = true;
    updateDisplayedSeries(m_currentChartRangeDays);
    setOffline(true);
    const SeriesCoverage coverage = Analyzer::computeCoverage(m_sourceSeries);
    if (coverage.ok) {
      setBanner("Offline: showing local history (" + formatCoverageSpan(coverage) + ")");
    } else {
      setBanner("Offline: showing local history");
    }
  } catch (const std::exception& ex) {
    setOffline(true);
    setBanner("DB error: exception while reading: " + QString::fromUtf8(ex.what()));
  } catch (...) {
    setOffline(true);
    setBanner("DB error: unknown exception while reading");
  }
}

// Czyści aktualny overlay mapy i unieważnia wszystkie starsze asynchroniczne odpowiedzi.
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

// Rozpoczyna zbiorcze kolorowanie mapy dla aktualnie widocznych stacji.
void AppController::refreshMapMeasurements(const QVariantList& stationIds, const QString& paramCode) {
  // Każde nowe odświeżenie dostaje świeży token, żeby starsze odpowiedzi
  // nie nadpisywały nowszego stanu po szybkim klikaniu użytkownika.
  ++m_mapRequestToken;
  const quint64 token = m_mapRequestToken;

  if (m_stationViewOffline) {
    if (!m_mapStationMetrics.isEmpty()) {
      m_mapStationMetrics.clear();
      emit mapStationMetricsChanged();
    }
    if (!m_mapMetricRange.isEmpty()) {
      m_mapMetricRange.clear();
      emit mapMetricRangeChanged();
    }
    setMapOverlayStatus("Map coloring is available only in ONLINE view.");
    return;
  }

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
  // Na starcie każda widoczna stacja dostaje stan "loading", żeby UI mogło to od razu pokazać.
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

  // Ta funkcja kończy obsługę pojedynczej stacji i aktualizuje stan całej paczki.
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

  // Dla każdej stacji pobieramy sensory, a potem szukamy pomiarów tylko dla zgodnego parametru.
  for (int stationId : uniqueIds) {
    m_gios.fetchSensorsForStation(
      stationId,
      [this, state, stationId, finishStation](int, QVariantList sensors) {
        if (state->token != m_mapRequestToken) return;

        // Szukamy tylko sensora pasującego do aktualnie wybranego parametru.
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

            // Do mapy bierzemy tylko ostatni dostępny punkt, bo on reprezentuje "stan teraz".
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
