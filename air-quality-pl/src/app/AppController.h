#pragma once
#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QDateTime>
#include "../storage/LocalDb.h"
#include "../net/GiosClient.h"
#include "../model/Types.h"

/**
 * Koordynuje współpracę UI, zdalnego API i lokalnej historii.
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
   * Tworzy główny kontroler aplikacji.
   */
  explicit AppController(QObject* parent = nullptr);

  /**
   * Zwraca bieżący tekst statusu widoczny w nagłówku.
   */
  QString banner() const { return m_banner; }
  /**
   * Informuje, czy aplikacja działa obecnie w trybie offline.
   */
  bool offline() const { return m_offline; }
  /**
   * Informuje, czy dla wybranego sensora istnieje lokalna historia.
   */
  bool historyAvailable() const { return m_historyAvailable; }
  /**
   * Zwraca identyfikator aktualnie wybranego sensora.
   */
  int currentSensorId() const { return m_currentSensorId; }
  /**
   * Zwraca kod aktualnie wybranego parametru pomiarowego.
   */
  QString currentParamCode() const { return m_currentParamCode; }
  /**
   * Zwraca ostatnie wartości używane do kolorowania markerów na mapie.
   */
  QVariantMap mapStationMetrics() const { return m_mapStationMetrics; }
  /**
   * Zwraca zakres wartości aktualnej warstwy kolorowania mapy.
   */
  QVariantMap mapMetricRange() const { return m_mapMetricRange; }
  /**
   * Zwraca opisowy status aktualnej warstwy mapy.
   */
  QString mapOverlayStatus() const { return m_mapOverlayStatus; }

  /**
   * Zwraca aktualnie załadowaną listę stacji.
   */
  QVariantList stations() const { return m_stations; }
  /**
   * Zwraca aktualnie załadowaną listę sensorów.
   */
  QVariantList sensors() const { return m_sensors; }

  /**
   * Zwraca punkty wykresu udostępniane do QML.
   */
  QVariantList chartPoints() const { return m_chartPoints; }
  /**
   * Zwraca statystyki wyliczone dla bieżącej serii.
   */
  QVariantMap stats() const { return m_stats; }

  /**
   * Pobiera listę stacji z API.
   */
  Q_INVOKABLE void refreshStations();
  /**
   * Pobiera sensory dla wybranej stacji.
   */
  Q_INVOKABLE void loadSensors(int stationId);
  /**
   * Pobiera dane pomiarowe dla sensora.
   */
  Q_INVOKABLE void loadOnline(int sensorId);

  /**
   * Zapisuje bieżącą serię do lokalnej bazy JSON.
   */
  Q_INVOKABLE void saveCurrentToDb();
  /**
   * Wczytuje historię z lokalnej bazy dla sensora i zakresu czasu.
   */
  Q_INVOKABLE void loadHistory(int sensorId, const QString& fromIso, const QString& toIso);
  /**
   * Wczytuje lokalną historię dla wybranego sensora z ostatnich N dni.
   */
  Q_INVOKABLE void loadCurrentHistory(int days);
  /**
   * Odświeża warstwę kolorowania mapy dla widocznych stacji i wybranego parametru.
   * W środku uruchamianych jest wiele asynchronicznych zapytań, a wynik jest agregowany.
   */
  Q_INVOKABLE void refreshMapMeasurements(const QVariantList& stationIds, const QString& paramCode);
  /**
   * Czyści bieżące wartości warstwy mapy.
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
   * Ustawia tekst banera i emituje sygnał zmiany.
   */
  void setBanner(QString b);
  /**
   * Ustawia tryb online/offline.
   */
  void setOffline(bool v);
  /**
   * Aktualizuje informację o dostępności lokalnej historii.
   */
  void setHistoryAvailable(bool v);
  /**
   * Zapamiętuje identyfikator bieżącego sensora.
   */
  void setCurrentSensorId(int sensorId);
  /**
   * Zapamiętuje kod bieżącego parametru.
   */
  void setCurrentParamCode(QString paramCode);
  /**
   * Zwraca kod parametru dla podanego sensora, jeśli jest znany.
   */
  QString sensorParamCode(int sensorId) const;
  /**
   * Buduje czytelną etykietę parametru do UI.
   */
  QString describeParamCode(const QString& paramCode) const;
  /**
   * Ustawia tekst statusu warstwy mapy.
   */
  void setMapOverlayStatus(QString status);
  /**
   * Przelicza zakres wartości widocznych na mapie.
   */
  void updateMapMetricRange();
  /**
   * Zapisuje jeden wpis wartości dla stacji na mapie.
   */
  void setMapStationMetric(int stationId, QVariantMap metric);

  /**
   * Odtwarza punkty domenowe z listy przekazywanej do QML.
   */
  void setChartFromVariantPoints(const QVariantList& pts);
  /**
   * Wylicza statystyki dla serii punktów.
   */
  void setStatsFromPoints(const QVector<MeasurementPoint>& pts);
  /**
   * Sprawdza ponownie, czy istnieje lokalna historia dla wybranego sensora.
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
