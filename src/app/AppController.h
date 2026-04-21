#pragma once
#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QDateTime>
#include "../storage/LocalDb.h"
#include "../net/GiosClient.h"
#include "../model/Types.h"

/**
 * Coordinates the UI, the remote API, and locally stored history.
 */
class AppController : public QObject {
  Q_OBJECT

  // The properties below are exposed to QML so the interface can react
  // automatically to state changes coming from C++.
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
  Q_PROPERTY(QString chartRangeInfo READ chartRangeInfo NOTIFY chartRangeInfoChanged)
  Q_PROPERTY(int chartRangeMaxDays READ chartRangeMaxDays NOTIFY chartRangeMaxDaysChanged)

public:
  /**
   * Creates the main application controller.
   */
  explicit AppController(QObject* parent = nullptr);

  /**
   * Returns the current status text shown in the header.
   */
  QString banner() const { return m_banner; }
  /**
   * Returns whether the application is currently operating in offline mode.
   */
  bool offline() const { return m_offline; }
  /**
   * Returns whether local history exists for the selected sensor.
   */
  bool historyAvailable() const { return m_historyAvailable; }
  /**
   * Returns the identifier of the currently selected sensor.
   */
  int currentSensorId() const { return m_currentSensorId; }
  /**
   * Returns the code of the currently selected measurement parameter.
   */
  QString currentParamCode() const { return m_currentParamCode; }
  /**
   * Returns the latest values used to color markers on the map.
   */
  QVariantMap mapStationMetrics() const { return m_mapStationMetrics; }
  /**
   * Returns the value range of the current map-coloring overlay.
   */
  QVariantMap mapMetricRange() const { return m_mapMetricRange; }
  /**
   * Returns a descriptive status string for the current map overlay.
   */
  QString mapOverlayStatus() const { return m_mapOverlayStatus; }

  /**
   * Returns the currently loaded station list.
   */
  QVariantList stations() const { return m_stations; }
  /**
   * Returns the currently loaded sensor list.
   */
  QVariantList sensors() const { return m_sensors; }

  /**
   * Returns chart points exposed to QML.
   */
  QVariantList chartPoints() const { return m_chartPoints; }
  /**
   * Returns statistics computed for the current series.
   */
  QVariantMap stats() const { return m_stats; }
  /**
   * Returns a user-facing description of the actual data range available on the chart.
   */
  QString chartRangeInfo() const { return m_chartRangeInfo; }
  /**
   * Returns the current upper limit for the selectable chart range.
   */
  int chartRangeMaxDays() const { return m_chartRangeMaxDays; }

  /**
   * Downloads the station list from the API.
   */
  Q_INVOKABLE void refreshStations();
  /**
   * Downloads sensors for the selected station.
   */
  Q_INVOKABLE void loadSensors(int stationId);
  /**
   * Downloads measurement data for a sensor.
   * The day range determines which part of the series should be shown on the chart.
   */
  Q_INVOKABLE void loadOnline(int sensorId, int days);
  /**
   * Changes the day range for the currently loaded series shown on the chart.
   */
  Q_INVOKABLE void applyCurrentChartRange(int days);

  /**
   * Saves the current series to the local JSON database.
   */
  Q_INVOKABLE void saveCurrentToDb();
  /**
   * Loads history from the local database for a sensor and time range.
   */
  Q_INVOKABLE void loadHistory(int sensorId, const QString& fromIso, const QString& toIso);
  /**
   * Loads local history for the selected sensor from the last N days.
   */
  Q_INVOKABLE void loadCurrentHistory(int days);
  /**
   * Refreshes the map-coloring overlay for visible stations and the selected parameter.
   * Internally this starts many asynchronous requests and aggregates the result.
   */
  Q_INVOKABLE void refreshMapMeasurements(const QVariantList& stationIds, const QString& paramCode);
  /**
   * Clears the current map-overlay values.
   */
  Q_INVOKABLE void clearMapMeasurements();

signals:
  // Signals notify QML that a specific part of the state has changed.
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
  void chartRangeInfoChanged();
  void chartRangeMaxDaysChanged();

private:
  // Private helper methods keep the controller logic organized and hide
  // state-update details from the user interface.
  /**
   * Sets the banner text and emits the corresponding change signal.
   */
  void setBanner(QString b);
  /**
   * Sets the current description of the data range available on the chart.
   */
  void setChartRangeInfo(QString info);
  /**
   * Sets the current upper limit for the chart-range selector.
   */
  void setChartRangeMaxDays(int days);
  /**
   * Updates the online/offline flag.
   */
  void setOffline(bool v);
  /**
   * Updates the local-history availability flag.
   */
  void setHistoryAvailable(bool v);
  /**
   * Stores the identifier of the current sensor.
   */
  void setCurrentSensorId(int sensorId);
  /**
   * Stores the code of the current parameter.
   */
  void setCurrentParamCode(QString paramCode);
  /**
   * Returns the parameter code for a given sensor when it is known.
   */
  QString sensorParamCode(int sensorId) const;
  /**
   * Builds a readable parameter label for the UI.
   */
  QString describeParamCode(const QString& paramCode) const;
  /**
   * Sets the status text for the map overlay.
   */
  void setMapOverlayStatus(QString status);
  /**
   * Recomputes the value range currently visible on the map.
   */
  void updateMapMetricRange();
  /**
   * Stores one value entry for a station on the map.
   */
  void setMapStationMetric(int stationId, QVariantMap metric);

  /**
   * Converts the domain series into chart points passed to QML.
   */
  void setChartFromPoints(const QVector<MeasurementPoint>& pts);
  /**
   * Returns only the tail of the series covering the last N days.
   */
  QVector<MeasurementPoint> filterSeriesToLastDays(const QVector<MeasurementPoint>& pts, int days) const;
  /**
   * Updates the visible series and statistics from the stored source series.
   */
  void updateDisplayedSeries(int days);
  /**
   * Computes statistics for the given series of points.
   */
  void setStatsFromPoints(const QVector<MeasurementPoint>& pts);
  /**
   * Rechecks whether local history exists for the selected sensor.
   */
  void refreshHistoryAvailability();
  /**
   * Refreshes the user-facing description of the actual data coverage.
   */
  void refreshChartRangeInfo();
  /**
   * Refreshes the maximum chart-range value allowed by the current source data.
   */
  void refreshChartRangeMaxDays();

  QString m_banner;
  bool m_offline = true;
  bool m_historyAvailable = false;

  // Data exposed directly to the UI.
  QVariantList m_stations;
  QVariantList m_sensors;

  QVariantList m_chartPoints;
  QVariantMap m_stats;
  QString m_chartRangeInfo;
  int m_chartRangeMaxDays = 30;
  QVariantMap m_mapStationMetrics;
  QVariantMap m_mapMetricRange;
  QString m_mapOverlayStatus;

  // Current application context.
  int m_currentSensorId = -1;
  QString m_currentParamCode;

  // m_sourceSeries stores the full source series, while m_currentSeries
  // stores only the part currently displayed on the chart.
  QVector<MeasurementPoint> m_sourceSeries;
  QVector<MeasurementPoint> m_currentSeries;
  int m_currentChartRangeDays = 7;
  bool m_showingLocalHistory = false;

  // Token used to discard stale map responses after a filter or sensor change.
  quint64 m_mapRequestToken = 0;

  // External dependencies used by the controller.
  LocalDb m_db;
  GiosClient m_gios;
};
