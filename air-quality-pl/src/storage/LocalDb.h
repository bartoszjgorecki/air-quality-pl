#pragma once
#include <QString>
#include <QVector>
#include <QDateTime>
#include "../model/Types.h"

/**
 * Zapewnia lokalny zapis i odczyt pomiarów w pliku JSON.
 */
class LocalDb {
public:
  /**
   * Tworzy obiekt obsługujący bazę pod podaną ścieżką.
   */
  explicit LocalDb(QString path);

  /**
   * Gwarantuje istnienie pliku bazy i poprawny kształt głównego obiektu JSON.
   */
  bool ensureExists(QString* err) const;

  /**
   * Wczytuje zapisaną historię dla sensora z podanego zakresu czasu.
   */
  QVector<MeasurementPoint> loadHistory(
    int sensorId, const QDateTime& from, const QDateTime& to, QString* err
  ) const;

  /**
   * Wstawia lub aktualizuje serię pomiarową dla sensora.
   */
  bool upsertSeries(
    int sensorId, const QString& paramCode, const QVector<MeasurementPoint>& pts, QString* err
  ) const;

  /**
   * Zwraca informację, czy dla sensora zapisano już jakąkolwiek serię.
   */
  bool hasAnySeries(int sensorId) const;

private:
  QString m_path;
};
