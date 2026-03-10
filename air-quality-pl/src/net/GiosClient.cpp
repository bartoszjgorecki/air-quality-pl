#include "GiosClient.h"

#include "GiosParsers.h"

#include <QDir>
#include <QFile>
#include <QFutureWatcher>
#include <QJsonParseError>
#include <QNetworkReply>
#include <QPointer>
#include <QSet>
#include <QTimer>
#include <QUrlQuery>

#include <QtConcurrent>

#include <exception>
#include <memory>
#include <utility>

namespace {

struct JsonTaskResult {
  QJsonDocument doc;
  QByteArray body;
  QString errorMessage;
};

JsonTaskResult parseJsonDocument(QByteArray body) {
  JsonTaskResult result;
  result.body = std::move(body);

  QJsonParseError parseError;
  result.doc = QJsonDocument::fromJson(result.body, &parseError);
  if (parseError.error != QJsonParseError::NoError) {
    result.errorMessage = QString("JSON parse error: %1").arg(parseError.errorString());
  }

  return result;
}

void dumpDebugPayload(const QString& fileName, const QByteArray& raw) {
  QDir().mkpath(QDir::current().filePath("debug"));
  QFile dump(QDir::current().filePath(QString("debug/%1").arg(fileName)));
  if (dump.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    dump.write(raw);
  }
}

}  // namespace

GiosClient::GiosClient(QObject* parent)
  : QObject(parent),
    m_baseUrl("https://api.gios.gov.pl/pjp-api/v1/rest"),
    m_timeoutMs(12000) {}

void GiosClient::getJson(const QUrl& url, std::function<void(const QJsonDocument&, const QByteArray&)>&& onOk) {
  QNetworkRequest req(url);
  req.setHeader(QNetworkRequest::UserAgentHeader, "air-quality-pl/1.0");
  req.setRawHeader("Accept", "application/ld+json, application/json;q=0.9, */*;q=0.1");

  QNetworkReply* reply = m_nam.get(req);

  QTimer* timer = new QTimer(reply);
  timer->setSingleShot(true);
  QObject::connect(timer, &QTimer::timeout, reply, [reply]() { reply->abort(); });
  timer->start(m_timeoutMs);

  QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, onOk = std::move(onOk)]() mutable {
    const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const auto networkError = reply->error();
    const auto body = reply->readAll();

    if (networkError != QNetworkReply::NoError) {
      emit error(QString("Network error: %1 (HTTP %2)").arg(reply->errorString()).arg(http));
      reply->deleteLater();
      return;
    }

    auto* watcher = new QFutureWatcher<JsonTaskResult>(this);
    QPointer<GiosClient> guard(this);

    QObject::connect(watcher, &QFutureWatcher<JsonTaskResult>::finished, this,
                     [guard, watcher, onOk = std::move(onOk)]() mutable {
      const JsonTaskResult result = watcher->result();
      watcher->deleteLater();

      if (!guard) return;

      if (!result.errorMessage.isEmpty()) {
        emit guard->error(result.errorMessage);
        return;
      }

      try {
        onOk(result.doc, result.body);
      } catch (const std::exception& ex) {
        emit guard->error("Unexpected client error: " + QString::fromUtf8(ex.what()));
      } catch (...) {
        emit guard->error("Unexpected client error");
      }
    });

    watcher->setFuture(QtConcurrent::run([body]() mutable {
      return parseJsonDocument(std::move(body));
    }));

    reply->deleteLater();
  });
}

void GiosClient::fetchStations() {
  QUrl url(m_baseUrl + "/station/findAll");
  QUrlQuery query;
  query.addQueryItem("page", "0");
  query.addQueryItem("size", "200");
  url.setQuery(query);

  auto allStations = std::make_shared<QVariantList>();
  auto seenIds = std::make_shared<QSet<int>>();

  auto fetchNext = std::make_shared<std::function<void(QUrl)>>();
  *fetchNext = [this, allStations, seenIds, fetchNext](QUrl current) {
    getJson(current, [this, allStations, seenIds, fetchNext, current](const QJsonDocument& doc, const QByteArray& raw) {
      dumpDebugPayload("stations_raw.json", raw);

      const auto page = GiosParsers::parseStationsPage(doc, current);
      for (const auto& item : page.stations) {
        const QVariantMap station = item.toMap();
        const int id = station.value("id").toInt();
        if (id == 0 || seenIds->contains(id)) continue;
        seenIds->insert(id);
        allStations->push_back(station);
      }

      if (page.reachedEnd) {
        emit stationsReady(*allStations);
        return;
      }

      (*fetchNext)(QUrl(page.nextUrl));
    });
  };

  (*fetchNext)(url);
}

void GiosClient::fetchSensors(int stationId) {
  fetchSensorsForStation(
    stationId,
    [this](int, QVariantList sensors) {
      emit sensorsReady(std::move(sensors));
    },
    [this](QString message) {
      emit error(std::move(message));
    }
  );
}

void GiosClient::fetchSensorsForStation(
  int stationId,
  std::function<void(int, QVariantList)> onReady,
  std::function<void(QString)> onError
) {
  const QUrl url(m_baseUrl + "/station/sensors/" + QString::number(stationId));
  getJson(url, [stationId, onReady = std::move(onReady), onError = std::move(onError)](const QJsonDocument& doc, const QByteArray& raw) mutable {
    dumpDebugPayload("sensors_raw.json", raw);
    try {
      onReady(stationId, GiosParsers::parseSensors(doc));
    } catch (const std::exception& ex) {
      if (onError) onError("Sensors parse error: " + QString::fromUtf8(ex.what()));
    } catch (...) {
      if (onError) onError("Sensors parse error");
    }
  });
}

void GiosClient::fetchMeasurements(int sensorId) {
  fetchMeasurementsForSensor(
    sensorId,
    [this](int readySensorId, QString paramCode, QVariantList points) {
      emit measurementsReady(readySensorId, std::move(paramCode), std::move(points));
    },
    [this](QString message) {
      emit error(std::move(message));
    }
  );
}

void GiosClient::fetchMeasurementsForSensor(
  int sensorId,
  std::function<void(int, QString, QVariantList)> onReady,
  std::function<void(QString)> onError
) {
  QUrl url(m_baseUrl + "/data/getData/" + QString::number(sensorId));
  QUrlQuery query;
  query.addQueryItem("page", "0");
  query.addQueryItem("size", "500");
  url.setQuery(query);

  getJson(url, [sensorId, onReady = std::move(onReady), onError = std::move(onError)](const QJsonDocument& doc, const QByteArray& raw) mutable {
    dumpDebugPayload("measurements_raw.json", raw);

    try {
      const auto series = GiosParsers::parseMeasurements(doc);
      onReady(sensorId, series.paramCode, series.points);
    } catch (const std::exception& ex) {
      if (onError) onError("Measurements parse error: " + QString::fromUtf8(ex.what()));
    } catch (...) {
      if (onError) onError("Measurements parse error");
    }
  });
}
