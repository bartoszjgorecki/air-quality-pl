#pragma once
#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QDateTime>
#include "../storage/LocalDb.h"
#include "../net/GiosClient.h"
#include "../model/Types.h"

/**
 * Koordynuje interfejs, zdalne API oraz lokalnie zapisaną historię.
 */
class AppController : public QObject {
  Q_OBJECT

  // Właściwości poniżej są wystawione do QML, aby interfejs mógł automatycznie
  // reagować na zmiany stanu pochodzące z warstwy C++.
  Q_PROPERTY(QString banner READ banner NOTIFY bannerChanged)
  Q_PROPERTY(bool offline READ offline NOTIFY offlineChanged)
  Q_PROPERTY(bool stationViewOffline READ stationViewOffline NOTIFY stationViewModeChanged)
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
   * Tworzy główny kontroler aplikacji.
   */
  explicit AppController(QObject* parent = nullptr);

  /**
   * Zwraca aktualny tekst statusu pokazywany w nagłówku.
   */
  QString banner() const { return m_banner; }
  /**
   * Zwraca informację, czy aplikacja działa obecnie w trybie offline.
   */
  bool offline() const { return m_offline; }
  /**
   * Zwraca informację, czy lista stacji jest aktualnie pokazywana w trybie lokalnym.
   */
  bool stationViewOffline() const { return m_stationViewOffline; }
  /**
   * Zwraca informację, czy dla wybranego sensora istnieje historia lokalna.
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
   * Zwraca najnowsze wartości używane do kolorowania markerów na mapie.
   */
  QVariantMap mapStationMetrics() const { return m_mapStationMetrics; }
  /**
   * Zwraca zakres wartości aktualnej warstwy kolorowania mapy.
   */
  QVariantMap mapMetricRange() const { return m_mapMetricRange; }
  /**
   * Zwraca opisowy komunikat statusu dla bieżącej warstwy mapy.
   */
  QString mapOverlayStatus() const { return m_mapOverlayStatus; }

  /**
   * Zwraca aktualnie wczytaną listę stacji.
   */
  QVariantList stations() const { return m_stationViewOffline ? m_offlineStations : m_onlineStations; }
  /**
   * Zwraca aktualnie wczytaną listę sensorów.
   */
  QVariantList sensors() const { return m_sensors; }

  /**
   * Zwraca punkty wykresu udostępniane do QML.
   */
  QVariantList chartPoints() const { return m_chartPoints; }
  /**
   * Zwraca statystyki policzone dla bieżącej serii.
   */
  QVariantMap stats() const { return m_stats; }
  /**
   * Zwraca opis rzeczywistego zakresu danych dostępnych aktualnie na wykresie.
   */
  QString chartRangeInfo() const { return m_chartRangeInfo; }
  /**
   * Zwraca bieżącą górną granicę wybieralnego zakresu wykresu.
   */
  int chartRangeMaxDays() const { return m_chartRangeMaxDays; }

  /**
   * Pobiera listę stacji z API.
   */
  Q_INVOKABLE void refreshStations();
  /**
   * Przełącza widok listy stacji między lokalną bazą a pełną listą z API.
   */
  Q_INVOKABLE void toggleStationViewMode();
  /**
   * Pobiera sensory dla wybranej stacji.
   */
  Q_INVOKABLE void loadSensors(int stationId);
  /**
   * Pobiera dane pomiarowe dla wskazanego sensora.
   * Zakres dni określa, która część serii ma zostać pokazana na wykresie.
   */
  Q_INVOKABLE void loadOnline(int sensorId, int days);
  /**
   * Ładuje dane sensora zgodnie z aktualnym trybem widoku stacji:
   * z lokalnej bazy w trybie offline albo z API w trybie online.
   */
  Q_INVOKABLE void loadSensorData(int sensorId, int days);
  /**
   * Zmienia zakres dni dla aktualnie wczytanej serii pokazywanej na wykresie.
   */
  Q_INVOKABLE void applyCurrentChartRange(int days);

  /**
   * Zapisuje bieżącą serię do lokalnej bazy JSON.
   */
  Q_INVOKABLE void saveCurrentToDb();
  /**
   * Wczytuje historię z lokalnej bazy dla sensora i podanego zakresu czasu.
   */
  Q_INVOKABLE void loadHistory(int sensorId, const QString& fromIso, const QString& toIso);
  /**
   * Wczytuje historię lokalną dla wybranego sensora z ostatnich N dni.
   */
  Q_INVOKABLE void loadCurrentHistory(int days);
  /**
   * Odświeża warstwę kolorowania mapy dla widocznych stacji i wybranego parametru.
   * Wewnętrznie uruchamia wiele żądań asynchronicznych i agreguje wynik.
   */
  Q_INVOKABLE void refreshMapMeasurements(const QVariantList& stationIds, const QString& paramCode);
  /**
   * Czyści bieżące wartości warstwy mapy.
   */
  Q_INVOKABLE void clearMapMeasurements();

signals:
  // Sygnały informują QML, że zmieniła się konkretna część stanu aplikacji.
  void bannerChanged();
  void offlineChanged();
  void stationViewModeChanged();
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
  enum class PendingRequest {
    None,
    Stations,
    Sensors,
    Measurements
  };

  // Prywatne metody pomocnicze porządkują logikę kontrolera i ukrywają
  // szczegóły aktualizacji stanu przed interfejsem użytkownika.
  /**
   * Ustawia tekst banera i emituje odpowiadający mu sygnał zmiany.
   */
  void setBanner(QString b);
  /**
   * Ustawia bieżący opis zakresu danych dostępnych na wykresie.
   */
  void setChartRangeInfo(QString info);
  /**
   * Ustawia bieżącą górną granicę selektora zakresu wykresu.
   */
  void setChartRangeMaxDays(int days);
  /**
   * Aktualizuje flagę trybu online/offline.
   */
  void setOffline(bool v);
  /**
   * Przełącza tryb wyświetlania listy stacji.
   */
  void setStationViewOffline(bool v);
  /**
   * Aktualizuje flagę dostępności historii lokalnej.
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
   * Odświeża lokalną listę stacji dostępnych naprawdę w trybie offline.
   */
  void refreshOfflineStations();
  /**
   * Zwraca kod parametru dla podanego sensora, jeśli jest znany.
   */
  QString sensorParamCode(int sensorId) const;
  /**
   * Wyszukuje stację po identyfikatorze w znanych listach lokalnych i online.
   */
  QVariantMap stationById(int stationId) const;
  /**
   * Buduje czytelną etykietę parametru dla interfejsu.
   */
  QString describeParamCode(const QString& paramCode) const;
  /**
   * Ustawia tekst statusu dla warstwy mapy.
   */
  void setMapOverlayStatus(QString status);
  /**
   * Przelicza zakres wartości aktualnie widocznych na mapie.
   */
  void updateMapMetricRange();
  /**
   * Zapisuje jeden wpis wartości dla stacji na mapie.
   */
  void setMapStationMetric(int stationId, QVariantMap metric);

  /**
   * Zamienia serię domenową na punkty wykresu przekazywane do QML.
   */
  void setChartFromPoints(const QVector<MeasurementPoint>& pts);
  /**
   * Zwraca tylko końcowy fragment serii obejmujący ostatnie N dni.
   */
  QVector<MeasurementPoint> filterSeriesToLastDays(const QVector<MeasurementPoint>& pts, int days) const;
  /**
   * Aktualizuje widoczną serię i statystyki na podstawie zapisanej serii źródłowej.
   */
  void updateDisplayedSeries(int days);
  /**
   * Liczy statystyki dla podanej serii punktów.
   */
  void setStatsFromPoints(const QVector<MeasurementPoint>& pts);
  /**
   * Ponownie sprawdza, czy dla wybranego sensora istnieje historia lokalna.
   */
  void refreshHistoryAvailability();
  /**
   * Odświeża opis rzeczywistego pokrycia danych widoczny dla użytkownika.
   */
  void refreshChartRangeInfo();
  /**
   * Odświeża maksymalną wartość zakresu wykresu dozwoloną przez bieżące źródło danych.
   */
  void refreshChartRangeMaxDays();
  /**
   * Wczytuje lokalnie zapisaną listę stacji, gdy żądanie online zakończy się błędem.
   */
  bool loadStationsFromLocalCache(const QString& failureReason);
  /**
   * Wczytuje lokalnie zapisane sensory dla wybranej stacji, gdy żądanie online zakończy się błędem.
   */
  bool loadSensorsFromLocalCache(int stationId, const QString& failureReason);

  QString m_banner;
  bool m_offline = true;
  bool m_stationViewOffline = true;
  bool m_historyAvailable = false;

  // Dane udostępniane bezpośrednio do interfejsu.
  QVariantList m_offlineStations;
  QVariantList m_onlineStations;
  QVariantList m_sensors;

  QVariantList m_chartPoints;
  QVariantMap m_stats;
  QString m_chartRangeInfo;
  int m_chartRangeMaxDays = 30;
  QVariantMap m_mapStationMetrics;
  QVariantMap m_mapMetricRange;
  QString m_mapOverlayStatus;

  // Bieżący kontekst pracy aplikacji.
  int m_currentStationId = -1;
  int m_currentSensorId = -1;
  QString m_currentParamCode;

  // `m_sourceSeries` przechowuje pełną serię źródłową, a `m_currentSeries`
  // tylko tę część, która jest aktualnie widoczna na wykresie.
  QVector<MeasurementPoint> m_sourceSeries;
  QVector<MeasurementPoint> m_currentSeries;
  int m_currentChartRangeDays = 7;
  bool m_showingLocalHistory = false;

  // Znacznik służący do odrzucania przestarzałych odpowiedzi mapy po zmianie filtra albo sensora.
  quint64 m_mapRequestToken = 0;

  // Te pola mówią obsłudze błędów, które żądanie online się nie powiodło
  // i z jakich danych lokalnych należy skorzystać w trybie awaryjnym.
  PendingRequest m_pendingRequest = PendingRequest::None;
  int m_pendingStationId = -1;

  // Zewnętrzne zależności używane przez kontroler.
  LocalDb m_db;
  GiosClient m_gios;
};
