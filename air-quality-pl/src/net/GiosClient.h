#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QVariantList>
#include <QJsonDocument>
#include <functional>

/**
 * Odpowiada za komunikację z REST API GIOS.
 */
class GiosClient : public QObject {
  Q_OBJECT
public:
  /**
   * Tworzy klienta API.
   */
  explicit GiosClient(QObject* parent = nullptr);

  /**
   * Pobiera listę stacji pomiarowych.
   */
  Q_INVOKABLE void fetchStations();
  /**
   * Pobiera sensory dostępne dla stacji.
   */
  Q_INVOKABLE void fetchSensors(int stationId);
  /**
   * Pobiera sensory dla stacji i przekazuje je do callbacku.
   * Ta ścieżka jest używana przez mapę do równoległych odpytań wielu stacji.
   */
  void fetchSensorsForStation(
    int stationId,
    std::function<void(int, QVariantList)> onReady,
    std::function<void(QString)> onError = {}
  );
  /**
   * Pobiera pomiary dla wybranego sensora.
   */
  Q_INVOKABLE void fetchMeasurements(int sensorId);
  /**
   * Pobiera pomiary dla sensora i przekazuje je do callbacku.
   * Ta ścieżka jest używana przez mapę do równoległych odpytań wielu stacji.
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
   * Wysyła żądanie GET i parsuje odpowiedź do JSON.
   * Samo dekodowanie JSON odbywa się w wątku roboczym.
   */
  void getJson(const QUrl& url, std::function<void(const QJsonDocument&, const QByteArray&)>&& onOk);
};
