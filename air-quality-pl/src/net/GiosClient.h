#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QVariantList>
#include <QJsonDocument>
#include <functional>

/**
 * Wraps communication with the GIOS REST API.
 */
class GiosClient : public QObject {
  Q_OBJECT
public:
  /**
   * Creates an API client instance.
   */
  explicit GiosClient(QObject* parent = nullptr);

  /**
   * Downloads the list of monitoring stations.
   */
  Q_INVOKABLE void fetchStations();
  /**
   * Downloads sensors available for a station.
   */
  Q_INVOKABLE void fetchSensors(int stationId);
  /**
   * Downloads sensors for a station and forwards them to a callback.
   * Used by the map overlay to run per-station lookups concurrently.
   */
  void fetchSensorsForStation(
    int stationId,
    std::function<void(int, QVariantList)> onReady,
    std::function<void(QString)> onError = {}
  );
  /**
   * Downloads measurements for a sensor.
   */
  Q_INVOKABLE void fetchMeasurements(int sensorId);
  /**
   * Downloads measurements for a sensor and forwards them to a callback.
   * Used by the map overlay to run per-station lookups concurrently.
   */
  void fetchMeasurementsForSensor(
    int sensorId,
    std::function<void(int, QString, QVariantList)> onReady,
    std::function<void(QString)> onError = {}
  );

signals:
  void stationsReady(QVariantList stations);
  void sensorsReady(QVariantList sensors);
  void measurementsReady(int sensorId, QString paramCode, QVariantList points);
  void error(QString message);

private:
  QNetworkAccessManager m_nam;
  QString m_baseUrl;
  int m_timeoutMs;

  /**
   * Performs a GET request and parses the response body as JSON.
   * The JSON decoding step is executed on a worker thread.
   */
  void getJson(const QUrl& url, std::function<void(const QJsonDocument&, const QByteArray&)>&& onOk);
};
