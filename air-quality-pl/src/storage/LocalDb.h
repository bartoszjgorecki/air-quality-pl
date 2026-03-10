#pragma once
#include <QString>
#include <QVector>
#include <QDateTime>
#include "../model/Types.h"

/**
 * Provides JSON-backed local persistence for downloaded measurements.
 */
class LocalDb {
public:
  /**
   * Creates a local database wrapper for the given file path.
   */
  explicit LocalDb(QString path);

  /**
   * Ensures that the JSON database file exists and has the expected root shape.
   */
  bool ensureExists(QString* err) const;

  /**
   * Loads stored history for a sensor within a date range.
   */
  QVector<MeasurementPoint> loadHistory(
    int sensorId, const QDateTime& from, const QDateTime& to, QString* err
  ) const;

  /**
   * Inserts or updates a measurement series for a sensor.
   */
  bool upsertSeries(
    int sensorId, const QString& paramCode, const QVector<MeasurementPoint>& pts, QString* err
  ) const;

  /**
   * Returns whether any local series exists for the sensor.
   */
  bool hasAnySeries(int sensorId) const;

private:
  QString m_path;
};
