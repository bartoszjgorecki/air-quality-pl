#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QVariantList>
#include <QJsonDocument>
#include <functional>

/**
 * Obsługuje komunikację z REST API GIOS.
 * Ta klasa zna wyłącznie warstwę sieciową i nie zawiera logiki interfejsu.
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
   * Pobiera sensory dostępne dla wskazanej stacji.
   */
  Q_INVOKABLE void fetchSensors(int stationId);
  /**
   * Pobiera sensory stacji i przekazuje je do callbacku.
   * Ta ścieżka jest używana przez warstwę mapy do równoległych zapytań dla wielu stacji.
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
   * Pobiera pomiary sensora i przekazuje je do callbacku.
   * Ta ścieżka jest używana przez warstwę mapy do równoległych zapytań dla wielu stacji.
   */
  void fetchMeasurementsForSensor(
    int sensorId,
    std::function<void(int, QString, QVariantList)> onReady,
    std::function<void(QString)> onError = {}
  );

signals:
  // Sparsowane dane są przekazywane do AppControllera przez sygnały Qt.
  void stationsReady(QVariantList stations);
  void sensorsReady(QVariantList sensors);
  void measurementsReady(int sensorId, QString paramCode, QVariantList points);
  void error(QString message);

private:
  /// Główny obiekt Qt odpowiedzialny za wykonywanie żądań HTTP.
  QNetworkAccessManager m_nam;
  /// Bazowy adres REST API GIOS.
  QString m_baseUrl;
  /// Timeout pojedynczego żądania wyrażony w milisekundach.
  int m_timeoutMs;

  /**
   * Wysyła żądanie GET i parsuje odpowiedź jako JSON.
   * Samo dekodowanie JSON działa w wątku roboczym.
   */
  void getJson(const QUrl& url, std::function<void(const QJsonDocument&, const QByteArray&)>&& onOk);
};
