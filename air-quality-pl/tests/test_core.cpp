#include <QtTest>
#include <QFile>
#include <QJsonDocument>
#include <QTimeZone>

#include <stdexcept>

#include "analysis/Analyzer.h"
#include "net/GiosParsers.h"
#include "storage/LocalDb.h"

namespace {

QJsonDocument jsonFrom(const QByteArray& raw) {
  QJsonParseError parseError;
  const auto doc = QJsonDocument::fromJson(raw, &parseError);
  if (parseError.error != QJsonParseError::NoError) {
    throw std::runtime_error(parseError.errorString().toStdString());
  }
  return doc;
}

}  // namespace

class CoreTests : public QObject {
  Q_OBJECT

private slots:
  void analyzerComputesStats();
  void analyzerRejectsEmptySeries();
  void localDbStoresAndLoadsHistory();
  void localDbReportsInvalidJson();
  void parsersHandleLocalizedStationsPayload();
  void parsersHandleLocalizedSensorsPayload();
  void parsersHandleLocalizedMeasurementsPayload();
  void parsersRejectMalformedPayload();
};

void CoreTests::analyzerComputesStats() {
  const QDateTime base(QDate(2026, 3, 1), QTime(10, 0), QTimeZone::UTC);
  const QVector<MeasurementPoint> pts{
    {base, 10.0},
    {base.addSecs(3600), 12.0},
    {base.addSecs(7200), std::nullopt},
    {base.addSecs(10800), 18.0},
    {base.addSecs(14400), 20.0},
  };

  const Stats stats = Analyzer::compute(pts);

  QVERIFY(stats.ok);
  QCOMPARE(stats.count, 4);
  QCOMPARE(stats.missing, 1);
  QVERIFY(qAbs(stats.min - 10.0) < 0.001);
  QVERIFY(qAbs(stats.max - 20.0) < 0.001);
  QVERIFY(qAbs(stats.avg - 15.0) < 0.001);
  QCOMPARE(stats.minAt, base);
  QCOMPARE(stats.maxAt, base.addSecs(14400));
  QCOMPARE(stats.trend, QString("up"));
}

void CoreTests::analyzerRejectsEmptySeries() {
  const Stats stats = Analyzer::compute({});
  QVERIFY(!stats.ok);
  QCOMPARE(stats.count, 0);
  QCOMPARE(stats.missing, 0);
}

void CoreTests::localDbStoresAndLoadsHistory() {
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString dbPath = tempDir.filePath("db.json");
  LocalDb db(dbPath);
  QString err;

  QVERIFY(db.ensureExists(&err));
  QVERIFY2(err.isEmpty(), qPrintable(err));

  const QDateTime base(QDate(2026, 3, 1), QTime(0, 0), QTimeZone::UTC);
  const QVector<MeasurementPoint> firstSeries{
    {base.addSecs(3600), 11.0},
    {base.addSecs(7200), 13.5},
  };

  QVERIFY(db.upsertSeries(42, "PM10", firstSeries, &err));
  QVERIFY2(err.isEmpty(), qPrintable(err));
  QVERIFY(db.hasAnySeries(42));

  auto loaded = db.loadHistory(42, base, base.addDays(1), &err);
  QVERIFY2(err.isEmpty(), qPrintable(err));
  QCOMPARE(loaded.size(), 2);
  QVERIFY(loaded[0].value.has_value());
  QVERIFY(loaded[1].value.has_value());
  QVERIFY(qAbs(*loaded[0].value - 11.0) < 0.001);
  QVERIFY(qAbs(*loaded[1].value - 13.5) < 0.001);

  const QVector<MeasurementPoint> updateSeries{
    {base.addSecs(7200), 14.0},
    {base.addSecs(10800), 15.5},
  };

  QVERIFY(db.upsertSeries(42, "PM10", updateSeries, &err));
  QVERIFY2(err.isEmpty(), qPrintable(err));

  loaded = db.loadHistory(42, base, base.addDays(1), &err);
  QVERIFY2(err.isEmpty(), qPrintable(err));
  QCOMPARE(loaded.size(), 3);
  QVERIFY(qAbs(*loaded[1].value - 14.0) < 0.001);
  QVERIFY(qAbs(*loaded[2].value - 15.5) < 0.001);
}

void CoreTests::localDbReportsInvalidJson() {
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString dbPath = tempDir.filePath("db.json");
  QFile invalidFile(dbPath);
  QVERIFY(invalidFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
  invalidFile.write("{ definitely not valid json");
  invalidFile.close();

  LocalDb db(dbPath);
  QString err;

  const auto loaded = db.loadHistory(99, QDateTime::currentDateTimeUtc().addDays(-1),
                                     QDateTime::currentDateTimeUtc(), &err);
  QVERIFY(loaded.isEmpty());
  QVERIFY(!err.isEmpty());
  QVERIFY(err.contains("parse", Qt::CaseInsensitive) || err.contains("json", Qt::CaseInsensitive));

  err.clear();
  const bool saved = db.upsertSeries(99, "NO2", {}, &err);
  QVERIFY(!saved);
  QVERIFY(!err.isEmpty());
}

void CoreTests::parsersHandleLocalizedStationsPayload() {
  const auto doc = jsonFrom(R"JSON(
    {
      "Lista stacji pomiarowych": [
        {
          "Identyfikator stacji": 11,
          "Nazwa stacji": "Czerniawa",
          "WGS84 φ N": "50.912475",
          "WGS84 λ E": "15.312190",
          "Nazwa miasta": "Czerniawa",
          "Powiat": "lubański",
          "Województwo": "DOLNOŚLĄSKIE",
          "Ulica": "ul. Strażacka 7"
        }
      ],
      "links": {
        "next": "https://api.gios.gov.pl/pjp-api/v1/rest/station/findAll?page=1&size=200",
        "self": "https://api.gios.gov.pl/pjp-api/v1/rest/station/findAll?page=0&size=200"
      },
      "totalPages": 2
    }
  )JSON");

  const auto page = GiosParsers::parseStationsPage(
    doc, QUrl("https://api.gios.gov.pl/pjp-api/v1/rest/station/findAll?page=0&size=200"));

  QCOMPARE(page.stations.size(), 1);
  QCOMPARE(page.nextUrl, QString("https://api.gios.gov.pl/pjp-api/v1/rest/station/findAll?page=1&size=200"));
  QVERIFY(!page.reachedEnd);

  const auto station = page.stations.first().toMap();
  QCOMPARE(station.value("id").toInt(), 11);
  QCOMPARE(station.value("name").toString(), QString("Czerniawa"));
  QCOMPARE(station.value("city").toString(), QString("Czerniawa"));
}

void CoreTests::parsersHandleLocalizedSensorsPayload() {
  const auto doc = jsonFrom(R"JSON(
    {
      "Lista stanowisk pomiarowych dla podanej stacji": [
        {
          "Identyfikator stanowiska": 958,
          "Identyfikator stacji": 156,
          "Wskaźnik": "tlenek azotu",
          "Wskaźnik - wzór": "NO",
          "Wskaźnik - kod": "NO",
          "Id wskaźnika": 16
        }
      ]
    }
  )JSON");

  const auto sensors = GiosParsers::parseSensors(doc);
  QCOMPARE(sensors.size(), 1);

  const auto sensor = sensors.first().toMap();
  QCOMPARE(sensor.value("id").toInt(), 958);
  QCOMPARE(sensor.value("stationId").toInt(), 156);
  QCOMPARE(sensor.value("paramName").toString(), QString("tlenek azotu"));
  QCOMPARE(sensor.value("paramCode").toString(), QString("NO"));
  QCOMPARE(sensor.value("displayName").toString(), QString("Nitric oxide"));
}

void CoreTests::parsersHandleLocalizedMeasurementsPayload() {
  const auto doc = jsonFrom(R"JSON(
    {
      "Lista danych pomiarowych": [
        {"Kod stanowiska": "Test-NO2-1g", "Data": "2026-03-10 15:00:00", "Wartość": 7.4},
        {"Kod stanowiska": "Test-NO2-1g", "Data": "2026-03-10 13:00:00", "Wartość": 9.1},
        {"Kod stanowiska": "Test-NO2-1g", "Data": "2026-03-10 14:00:00", "Wartość": 6.2}
      ]
    }
  )JSON");

  const auto series = GiosParsers::parseMeasurements(doc);
  QCOMPARE(series.paramCode, QString("Test-NO2-1g"));
  QCOMPARE(series.points.size(), 3);

  const auto first = series.points.first().toMap();
  const auto last = series.points.last().toMap();
  QVERIFY(first.value("t").toLongLong() < last.value("t").toLongLong());
  QVERIFY(qAbs(first.value("v").toDouble() - 9.1) < 0.001);
  QVERIFY(qAbs(last.value("v").toDouble() - 7.4) < 0.001);
}

void CoreTests::parsersRejectMalformedPayload() {
  bool stationsThrown = false;
  bool sensorsThrown = false;
  bool measurementsThrown = false;

  try {
    GiosParsers::parseStationsPage(jsonFrom(R"JSON({"meta": {"x": 1}})JSON"),
                                   QUrl("https://api.gios.gov.pl/pjp-api/v1/rest/station/findAll?page=0&size=200"));
  } catch (const std::runtime_error&) {
    stationsThrown = true;
  }

  try {
    GiosParsers::parseSensors(jsonFrom(R"JSON({"meta": {"x": 1}})JSON"));
  } catch (const std::runtime_error&) {
    sensorsThrown = true;
  }

  try {
    GiosParsers::parseMeasurements(jsonFrom(R"JSON({"meta": {"x": 1}})JSON"));
  } catch (const std::runtime_error&) {
    measurementsThrown = true;
  }

  QVERIFY(stationsThrown);
  QVERIFY(sensorsThrown);
  QVERIFY(measurementsThrown);
}

QTEST_MAIN(CoreTests)

#include "test_core.moc"
