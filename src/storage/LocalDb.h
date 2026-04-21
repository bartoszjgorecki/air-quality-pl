#pragma once
#include <QString>
#include <QVector>
#include <QDateTime>
#include <QVariantList>
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
   * Podmienia zapisaną listę stacji na najnowszą migawkę pobraną online.
   */
  bool replaceStations(const QVariantList& stations, QString* err) const;
  /**
   * Wczytuje z lokalnej bazy zapisane sensory dla jednej stacji.
   */
  QVariantList loadSensorsForStation(int stationId, QString* err) const;
  /**
   * Dodaje lub aktualizuje zapisaną listę sensorów dla wybranej stacji.
   */
  bool upsertSensorsForStation(int stationId, const QVariantList& sensors, QString* err) const;

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
