#pragma once
#include <QString>
#include <QVector>
#include <QDateTime>
#include "../model/Types.h"

/**
 * Provides local persistence for measurements in a JSON file.
 * It is the simplest local "database" required by the project.
 */
class LocalDb {
public:
  /**
   * Creates a database wrapper for the given file path.
   */
  explicit LocalDb(QString path);

  /**
   * Ensures that the database file exists and has the expected root JSON shape.
   */
  bool ensureExists(QString* err) const;

  /**
   * Loads saved history for a sensor within the given time range.
   */
  QVector<MeasurementPoint> loadHistory(
    int sensorId, const QDateTime& from, const QDateTime& to, QString* err
  ) const;
  /**
   * Loads the full saved history for a sensor without applying a date filter.
   */
  QVector<MeasurementPoint> loadAllHistory(int sensorId, QString* err) const;

  /**
   * Inserts or updates a measurement series for a sensor.
   */
  bool upsertSeries(
    int sensorId, const QString& paramCode, const QVector<MeasurementPoint>& pts, QString* err
  ) const;

  /**
   * Returns whether any series has already been saved for the sensor.
   */
  bool hasAnySeries(int sensorId) const;

private:
  /// Path to the db.json file used by the application.
  QString m_path;
};
