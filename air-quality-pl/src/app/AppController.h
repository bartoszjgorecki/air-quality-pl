#pragma once
#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QDateTime>
#include "../storage/LocalDb.h"
#include "../net/GiosClient.h"
#include "../model/Types.h"

/**
 * Coordinates the UI, online API access and local history storage.
 */
class AppController : public QObject {
  Q_OBJECT

  Q_PROPERTY(QString banner READ banner NOTIFY bannerChanged)
  Q_PROPERTY(bool offline READ offline NOTIFY offlineChanged)
  Q_PROPERTY(bool historyAvailable READ historyAvailable NOTIFY historyAvailableChanged)
  Q_PROPERTY(int currentSensorId READ currentSensorId NOTIFY currentSensorIdChanged)
  Q_PROPERTY(QString currentParamCode READ currentParamCode NOTIFY currentParamCodeChanged)
  Q_PROPERTY(QVariantMap mapStationMetrics READ mapStationMetrics NOTIFY mapStationMetricsChanged)
  Q_PROPERTY(QVariantMap mapMetricRange READ mapMetricRange NOTIFY mapMetricRangeChanged)
  Q_PROPERTY(QString mapOverlayStatus READ mapOverlayStatus NOTIFY mapOverlayStatusChanged)

  Q_PROPERTY(QVariantList stations READ stations NOTIFY stationsChanged)
  Q_PROPERTY(QVariantList sensors READ sensors NOTIFY sensorsChanged)

  Q_PROPERTY(QVariantList chartPoints READ chartPoints NOTIFY chartPointsChanged)
  Q_PROPERTY(QVariantMap stats READ stats NOTIFY statsChanged)

public:
  /**
   * Creates the main application controller.
   */
  explicit AppController(QObject* parent = nullptr);

  /**
   * Returns the current status banner text.
   */
  QString banner() const { return m_banner; }
  /**
   * Returns whether the UI currently operates in offline mode.
   */
  bool offline() const { return m_offline; }
  /**
   * Returns whether local history exists for the selected sensor.
   */
  bool historyAvailable() const { return m_historyAvailable; }
  /**
   * Returns the currently selected sensor identifier.
   */
  int currentSensorId() const { return m_currentSensorId; }
  /**
   * Returns the current measurement parameter code.
   */
  QString currentParamCode() const { return m_currentParamCode; }
  /**
   * Returns the latest map metrics for visible stations.
   */
  QVariantMap mapStationMetrics() const { return m_mapStationMetrics; }
  /**
   * Returns the value range of the current map overlay.
   */
  QVariantMap mapMetricRange() const { return m_mapMetricRange; }
  /**
   * Returns the current status text for the map overlay.
   */
  QString mapOverlayStatus() const { return m_mapOverlayStatus; }

  /**
   * Returns the currently loaded stations list.
   */
  QVariantList stations() const { return m_stations; }
  /**
   * Returns the currently loaded sensors list.
   */
  QVariantList sensors() const { return m_sensors; }

  /**
   * Returns chart points exposed to QML.
   */
  QVariantList chartPoints() const { return m_chartPoints; }
  /**
   * Returns computed statistics for the current chart series.
   */
  QVariantMap stats() const { return m_stats; }

  /**
   * Downloads the list of monitoring stations from the remote API.
   */
  Q_INVOKABLE void refreshStations();
  /**
   * Downloads sensors for a station.
   */
  Q_INVOKABLE void loadSensors(int stationId);
  /**
   * Downloads measurements for a sensor.
   */
  Q_INVOKABLE void loadOnline(int sensorId);

  /**
   * Saves the currently loaded online series into the local JSON database.
   */
  Q_INVOKABLE void saveCurrentToDb();
  /**
   * Loads history from local storage for the given sensor and date range.
   */
  Q_INVOKABLE void loadHistory(int sensorId, const QString& fromIso, const QString& toIso);
  /**
   * Loads local history for the currently selected sensor from the last N days.
   */
  Q_INVOKABLE void loadCurrentHistory(int days);
  /**
   * Refreshes the map overlay for the visible stations and selected parameter.
   * The refresh fans out multiple asynchronous API calls and aggregates the result.
   */
  Q_INVOKABLE void refreshMapMeasurements(const QVariantList& stationIds, const QString& paramCode);
  /**
   * Clears map overlay values.
   */
  Q_INVOKABLE void clearMapMeasurements();

signals:
  void bannerChanged();
  void offlineChanged();
  void historyAvailableChanged();
  void currentSensorIdChanged();
  void currentParamCodeChanged();
  void mapStationMetricsChanged();
  void mapMetricRangeChanged();
  void mapOverlayStatusChanged();

  void stationsChanged();
  void sensorsChanged();

  void chartPointsChanged();
  void statsChanged();

private:
  /**
   * Updates the banner and notifies QML when it changes.
   */
  void setBanner(QString b);
  /**
   * Updates the online/offline mode flag.
   */
  void setOffline(bool v);
  /**
   * Updates local history availability for the selected sensor.
   */
  void setHistoryAvailable(bool v);
  /**
   * Updates the currently selected sensor identifier.
   */
  void setCurrentSensorId(int sensorId);
  /**
   * Updates the currently selected parameter code.
   */
  void setCurrentParamCode(QString paramCode);
  /**
   * Returns the measurement parameter code for the selected sensor, if known.
   */
  QString sensorParamCode(int sensorId) const;
  /**
   * Builds a readable label for a measurement parameter.
   */
  QString describeParamCode(const QString& paramCode) const;
  /**
   * Updates the current map overlay status text.
   */
  void setMapOverlayStatus(QString status);
  /**
   * Recomputes the range of map values.
   */
  void updateMapMetricRange();
  /**
   * Stores one map metric entry for a station.
   */
  void setMapStationMetric(int stationId, QVariantMap metric);

  /**
   * Converts QML chart points back into domain points.
   */
  void setChartFromVariantPoints(const QVariantList& pts);
  /**
   * Computes statistics for a points series.
   */
  void setStatsFromPoints(const QVector<MeasurementPoint>& pts);
  /**
   * Recomputes whether local history exists for the selected sensor.
   */
  void refreshHistoryAvailability();

  QString m_banner;
  bool m_offline = true;
  bool m_historyAvailable = false;

  QVariantList m_stations;
  QVariantList m_sensors;

  QVariantList m_chartPoints;
  QVariantMap m_stats;
  QVariantMap m_mapStationMetrics;
  QVariantMap m_mapMetricRange;
  QString m_mapOverlayStatus;

  int m_currentSensorId = -1;
  QString m_currentParamCode;
  QVector<MeasurementPoint> m_currentSeries;
  quint64 m_mapRequestToken = 0;

  LocalDb m_db;
  GiosClient m_gios;
};
