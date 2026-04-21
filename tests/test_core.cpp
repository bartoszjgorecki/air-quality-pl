#include <QtTest>
#include <QFile>
#include <QJsonDocument>
#include <QDir>
#include <QTimeZone>

#include <stdexcept>

#include "analysis/Analyzer.h"
#include "app/AppController.h"
#include "net/GiosParsers.h"
#include "storage/LocalDb.h"

namespace {

QJsonDocument jsonFrom(const QByteArray& raw) {
  // Pomocniczo zamieniamy tekst JSON na dokument i od razu zgłaszamy błąd testu,
  // jeśli wejście jest niepoprawne.
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
  void analyzerComputesCoverage();
  void analyzerRejectsEmptySeries();
  void localDbStoresAndLoadsHistory();
  void localDbLoadsFullSensorHistory();
  void localDbStoresAndLoadsStationCache();
  void localDbStoresAndLoadsSensorCache();
  void appControllerLoadsCachedStationsAtStartup();
  void localDbReportsInvalidJson();
  void parsersHandleLocalizedStationsPayload();
  void parsersHandleLocalizedSensorsPayload();
  void parsersTranslatePm10DisplayName();
  void parsersTranslateCarbonMonoxideDisplayName();
  void parsersHandleLocalizedMeasurementsPayload();
  void parsersRejectMalformedPayload();
};

void CoreTests::analyzerComputesStats() {
  // Seria zawiera także jedną brakującą wartość, żeby sprawdzić jej zliczanie.
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

void CoreTests::analyzerComputesCoverage() {
  const QDateTime base(QDate(2026, 4, 18), QTime(2, 0), QTimeZone::UTC);
  const QVector<MeasurementPoint> pts{
    {base, 19.0},
    {base.addSecs(12 * 3600), 21.0},
    {base.addSecs(57 * 3600), 20.5},
  };

  const SeriesCoverage coverage = Analyzer::computeCoverage(pts);

  QVERIFY(coverage.ok);
  QCOMPARE(coverage.pointCount, 3);
  QCOMPARE(coverage.firstAt, base);
  QCOMPARE(coverage.lastAt, base.addSecs(57 * 3600));
  QCOMPARE(coverage.spanHours, 57LL);
  QVERIFY(qAbs(coverage.spanDays - 2.375) < 0.001);
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

  // Drugi zapis powinien nadpisać duplikat daty i dopisać nowy punkt.
  QVERIFY(db.upsertSeries(42, "PM10", updateSeries, &err));
  QVERIFY2(err.isEmpty(), qPrintable(err));

  loaded = db.loadHistory(42, base, base.addDays(1), &err);
  QVERIFY2(err.isEmpty(), qPrintable(err));
  QCOMPARE(loaded.size(), 3);
  QVERIFY(qAbs(*loaded[1].value - 14.0) < 0.001);
  QVERIFY(qAbs(*loaded[2].value - 15.5) < 0.001);
}

void CoreTests::localDbLoadsFullSensorHistory() {
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString dbPath = tempDir.filePath("db.json");
  LocalDb db(dbPath);
  QString err;

  QVERIFY(db.ensureExists(&err));
  QVERIFY2(err.isEmpty(), qPrintable(err));

  const QDateTime base(QDate(2026, 3, 1), QTime(0, 0), QTimeZone::UTC);
  const QVector<MeasurementPoint> pts{
    {base.addSecs(3600), 11.0},
    {base.addSecs(7200), 13.5},
    {base.addDays(5), 21.0},
  };

  QVERIFY(db.upsertSeries(77, "PM10", pts, &err));
  QVERIFY2(err.isEmpty(), qPrintable(err));

  const auto loaded = db.loadAllHistory(77, &err);
  QVERIFY2(err.isEmpty(), qPrintable(err));
  QCOMPARE(loaded.size(), 3);
  QCOMPARE(loaded.first().dt, base.addSecs(3600));
  QCOMPARE(loaded.last().dt, base.addDays(5));
}

void CoreTests::localDbStoresAndLoadsStationCache() {
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString dbPath = tempDir.filePath("db.json");
  LocalDb db(dbPath);
  QString err;

  QVERIFY(db.ensureExists(&err));
  QVERIFY2(err.isEmpty(), qPrintable(err));

  QVariantList stations;
  stations.push_back(QVariantMap{
    {"id", 11},
    {"name", "Poznan, Polanka"},
    {"city", "Poznan"},
    {"province", "WIELKOPOLSKIE"},
    {"address", "Polanka 3"},
    {"lat", 52.404f},
    {"lon", 16.953f},
  });
  stations.push_back(QVariantMap{
    {"id", 12},
    {"name", "Warszawa, Marszalkowska"},
    {"city", "Warszawa"},
    {"province", "MAZOWIECKIE"},
    {"address", "Marszalkowska"},
    {"lat", 52.229f},
    {"lon", 21.012f},
  });

  QVERIFY(db.replaceStations(stations, &err));
  QVERIFY2(err.isEmpty(), qPrintable(err));

  const auto loaded = db.loadStations(&err);
  QVERIFY2(err.isEmpty(), qPrintable(err));
  QCOMPARE(loaded.size(), 2);
  QCOMPARE(loaded[0].toMap().value("id").toInt(), 11);
  QCOMPARE(loaded[0].toMap().value("city").toString(), QString("Poznan"));
  QCOMPARE(loaded[1].toMap().value("id").toInt(), 12);
}

void CoreTests::localDbStoresAndLoadsSensorCache() {
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString dbPath = tempDir.filePath("db.json");
  LocalDb db(dbPath);
  QString err;

  QVERIFY(db.ensureExists(&err));
  QVERIFY2(err.isEmpty(), qPrintable(err));

  QVariantList sensors;
  sensors.push_back(QVariantMap{
    {"id", 958},
    {"stationId", 156},
    {"paramName", "tlenek azotu"},
    {"paramCode", "NO"},
    {"displayName", "Nitric oxide"},
    {"paramFormula", "NO"},
    {"idParam", 16},
  });
  sensors.push_back(QVariantMap{
    {"id", 959},
    {"stationId", 156},
    {"paramName", "ozon"},
    {"paramCode", "O3"},
    {"displayName", "Ozone"},
    {"paramFormula", "O3"},
    {"idParam", 7},
  });

  QVERIFY(db.upsertSensorsForStation(156, sensors, &err));
  QVERIFY2(err.isEmpty(), qPrintable(err));

  const auto loaded = db.loadSensorsForStation(156, &err);
  QVERIFY2(err.isEmpty(), qPrintable(err));
  QCOMPARE(loaded.size(), 2);
  QCOMPARE(loaded[0].toMap().value("id").toInt(), 958);
  QCOMPARE(loaded[1].toMap().value("paramCode").toString(), QString("O3"));

  err.clear();
  const auto missing = db.loadSensorsForStation(999, &err);
  QVERIFY(missing.isEmpty());
  QVERIFY(!err.isEmpty());
}

void CoreTests::appControllerLoadsCachedStationsAtStartup() {
  QTemporaryDir tempDir;
  QVERIFY(tempDir.isValid());

  const QString originalCurrentPath = QDir::currentPath();
  QVERIFY(QDir().mkpath(tempDir.filePath("data")));

  LocalDb db(tempDir.filePath("data/db.json"));
  QString err;
  QVERIFY(db.ensureExists(&err));
  QVERIFY2(err.isEmpty(), qPrintable(err));

  QVariantList stations;
  stations.push_back(QVariantMap{
    {"id", 501},
    {"name", "Poznan, Polanka"},
    {"city", "Poznan"},
    {"province", "WIELKOPOLSKIE"},
    {"address", "Polanka 3"},
    {"lat", 52.404f},
    {"lon", 16.953f},
  });

  QVERIFY(db.replaceStations(stations, &err));
  QVERIFY2(err.isEmpty(), qPrintable(err));
  QVERIFY(QDir::setCurrent(tempDir.path()));

  {
    AppController controller;
    QCOMPARE(controller.stations().size(), 1);
    QCOMPARE(controller.stations().first().toMap().value("id").toInt(), 501);
    QVERIFY(controller.offline());
  }

  QVERIFY(QDir::setCurrent(originalCurrentPath));
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

void CoreTests::parsersTranslatePm10DisplayName() {
  const auto doc = jsonFrom(R"JSON(
    {
      "Lista stanowisk pomiarowych dla podanej stacji": [
        {
          "Identyfikator stanowiska": 1201,
          "Identyfikator stacji": 156,
          "Wskaźnik": "pył zawieszony PM10",
          "Wskaźnik - wzór": "PM10",
          "Wskaźnik - kod": "PM10",
          "Id wskaźnika": 69
        }
      ]
    }
  )JSON");

  const auto sensors = GiosParsers::parseSensors(doc);
  QCOMPARE(sensors.size(), 1);

  const auto sensor = sensors.first().toMap();
  QCOMPARE(sensor.value("paramCode").toString(), QString("PM10"));
  QCOMPARE(sensor.value("displayName").toString(), QString("Particulate matter PM10"));
}

void CoreTests::parsersTranslateCarbonMonoxideDisplayName() {
  const auto doc = jsonFrom(R"JSON(
    {
      "Lista stanowisk pomiarowych dla podanej stacji": [
        {
          "Identyfikator stanowiska": 1202,
          "Identyfikator stacji": 156,
          "Wskaźnik": "tlenek węgla",
          "Wskaźnik - wzór": "CO",
          "Wskaźnik - kod": "CO",
          "Id wskaźnika": 8
        }
      ]
    }
  )JSON");

  const auto sensors = GiosParsers::parseSensors(doc);
  QCOMPARE(sensors.size(), 1);

  const auto sensor = sensors.first().toMap();
  QCOMPARE(sensor.value("paramCode").toString(), QString("CO"));
  QCOMPARE(sensor.value("displayName").toString(), QString("Carbon monoxide"));
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
  // Każdy parser powinien odrzucić JSON bez wymaganej tablicy danych.
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
