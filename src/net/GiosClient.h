#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QVariantList>
#include <QJsonDocument>
#include <functional>

/**
 * Handles communication with the GIOS REST API.
 * This class knows only the networking layer and contains no UI logic.
 */
class GiosClient : public QObject {
  Q_OBJECT
public:
  /**
   * Creates the API client.
   */
  explicit GiosClient(QObject* parent = nullptr);

  /**
   * Downloads the list of monitoring stations.
   */
  Q_INVOKABLE void fetchStations();
  /**
   * Downloads the sensors available at a station.
   */
  Q_INVOKABLE void fetchSensors(int stationId);
  /**
   * Downloads station sensors and passes them to a callback.
   * This path is used by the map layer to query multiple stations in parallel.
   */
  void fetchSensorsForStation(
    int stationId,
    std::function<void(int, QVariantList)> onReady,
    std::function<void(QString)> onError = {}
  );
  /**
   * Downloads measurements for the selected sensor.
   */
  Q_INVOKABLE void fetchMeasurements(int sensorId);
  /**
   * Downloads sensor measurements and passes them to a callback.
   * This path is used by the map layer to query multiple stations in parallel.
   */
  void fetchMeasurementsForSensor(
    int sensorId,
    std::function<void(int, QString, QVariantList)> onReady,
    std::function<void(QString)> onError = {}
  );

signals:
  // Parsed data is forwarded to AppController through Qt signals.
  void stationsReady(QVariantList stations);
  void sensorsReady(QVariantList sensors);
  void measurementsReady(int sensorId, QString paramCode, QVariantList points);
  void error(QString message);

private:
  /// Main Qt object responsible for executing HTTP requests.
  QNetworkAccessManager m_nam;
  /// Base address of the GIOS REST API.
  QString m_baseUrl;
  /// Timeout for a single request in milliseconds.
  int m_timeoutMs;

  /**
   * Sends a GET request and parses the response as JSON.
   * JSON decoding itself runs on a worker thread.
   */
  void getJson(const QUrl& url, std::function<void(const QJsonDocument&, const QByteArray&)>&& onOk);
};
