#pragma once
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QVariantList>
#include <QVariantMap>
#include "../model/Types.h"

/**
 * Zapewnia lokalne zapisywanie danych w pliku JSON.
 * To najprostsza lokalna "baza danych" wymagana w projekcie.
 */
class LocalDb {
public:
  /**
   * Tworzy obiekt obsługujący bazę wskazaną przez podaną ścieżkę.
   */
  explicit LocalDb(QString path);

  /**
   * Gwarantuje istnienie pliku bazy oraz poprawny kształt głównego obiektu JSON.
   */
  bool ensureExists(QString* err) const;

  /**
   * Wczytuje z lokalnej bazy zapisaną listę stacji.
   */
  QVariantList loadStations(QString* err) const;
  /**
   * Wczytuje tylko te stacje, które naprawdę nadają się do pracy offline,
   * czyli mają zapisane sensory w lokalnej bazie.
   */
  QVariantList loadOfflineStations(QString* err) const;
  /**
   * Podmienia zapisaną listę stacji na najnowszą migawkę pobraną online.
   */
  bool replaceStations(const QVariantList& stations, QString* err) const;
  /**
   * Dodaje lub aktualizuje pojedynczą stację w lokalnej bazie.
   */
  bool upsertStation(const QVariantMap& station, QString* err) const;
  /**
   * Wczytuje z lokalnej bazy zapisane sensory dla jednej stacji.
   */
  QVariantList loadSensorsForStation(int stationId, QString* err) const;
  /**
   * Wczytuje tylko te zapisane sensory stacji, dla których istnieje lokalna historia pomiarów.
   */
  QVariantList loadSensorsWithSavedHistoryForStation(int stationId, QString* err) const;
  /**
   * Dodaje lub aktualizuje zapisaną listę sensorów dla wybranej stacji.
   */
  bool upsertSensorsForStation(int stationId, const QVariantList& sensors, QString* err) const;
  /**
   * Usuwa ze stacji lokalnych te wpisy, które nie mają zapisanych sensorów
   * i przez to nie są użyteczne w trybie offline.
   */
  bool pruneStationsWithoutCachedSensors(QString* err) const;

  /**
   * Wczytuje zapisaną historię sensora z podanego zakresu czasu.
   */
  QVector<MeasurementPoint> loadHistory(
    int sensorId, const QDateTime& from, const QDateTime& to, QString* err
  ) const;
  /**
   * Wczytuje pełną zapisaną historię sensora bez filtrowania po dacie.
   */
  QVector<MeasurementPoint> loadAllHistory(int sensorId, QString* err) const;

  /**
   * Dodaje lub aktualizuje serię pomiarową dla sensora.
   */
  bool upsertSeries(
    int sensorId, const QString& paramCode, const QVector<MeasurementPoint>& pts, QString* err
  ) const;

  /**
   * Zwraca informację, czy dla sensora zapisano już jakąkolwiek serię.
   */
  bool hasAnySeries(int sensorId) const;

private:
  /// Ścieżka do pliku `db.json` używanego przez aplikację.
  QString m_path;
};
