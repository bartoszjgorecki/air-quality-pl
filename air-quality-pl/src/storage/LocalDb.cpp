#include "LocalDb.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QMap>
#include <QSaveFile>
#include <algorithm>

static QJsonObject emptyDb() {
  // Zostawiamy miejsce na przyszłe typy danych, nawet jeśli dziś aktywnie
  // korzystamy głównie z tablicy "measurements".
  QJsonObject o;
  o["stations"] = QJsonArray{};
  o["sensors"] = QJsonArray{};
  o["measurements"] = QJsonArray{};
  o["aqIndex"] = QJsonArray{};
  return o;
}

static QJsonDocument readJsonDocument(QFile& file, QString* err) {
  QJsonParseError parseError;
  const auto doc = QJsonDocument::fromJson(file.readAll(), &parseError);
  if (parseError.error != QJsonParseError::NoError) {
    if (err) *err = "db.json parse error: " + parseError.errorString();
    return {};
  }
  return doc;
}

LocalDb::LocalDb(QString path) : m_path(std::move(path)) {}

bool LocalDb::ensureExists(QString* err) const {
  QFile f(m_path);
  if (f.exists()) return true;

  // Tworzymy katalog docelowy dopiero wtedy, gdy faktycznie jest potrzebny.
  QDir().mkpath(QFileInfo(m_path).absolutePath());

  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    if (err) *err = "cannot create db.json";
    return false;
  }

  QJsonDocument doc(emptyDb());
  f.write(doc.toJson(QJsonDocument::Indented));
  return true;
}

bool LocalDb::hasAnySeries(int sensorId) const {
  QFile f(m_path);
  if (!f.open(QIODevice::ReadOnly)) return false;

  QString err;
  const auto doc = readJsonDocument(f, &err);
  if (!doc.isObject()) return false;

  const auto arr = doc.object().value("measurements").toArray();
  // Wystarczy znaleźć dowolny wpis dla sensora, nie musimy odczytywać całej serii.
  for (const auto& v : arr) {
    const auto o = v.toObject();
    if (o.value("sensorId").toInt() == sensorId) return true;
  }
  return false;
}

QVector<MeasurementPoint> LocalDb::loadHistory(
  int sensorId, const QDateTime& from, const QDateTime& to, QString* err
) const {
  QVector<MeasurementPoint> out;

  QFile f(m_path);
  if (!f.open(QIODevice::ReadOnly)) {
    if (err) *err = "cannot open db.json";
    return out;
  }

  const auto doc = readJsonDocument(f, err);
  if (!doc.isObject()) {
    if (err && err->isEmpty()) *err = "db.json is not object";
    return out;
  }

  const auto root = doc.object();
  const auto measurements = root.value("measurements").toArray();

  for (const auto& entryVal : measurements) {
    const auto entry = entryVal.toObject();
    if (entry.value("sensorId").toInt() != sensorId) continue;

    const auto items = entry.value("items").toArray();
    for (const auto& itVal : items) {
      const auto it = itVal.toObject();

      const auto dt = QDateTime::fromString(it.value("date").toString(), Qt::ISODate);
      if (!dt.isValid()) continue;
      if (dt < from || dt > to) continue;

      MeasurementPoint p;
      p.dt = dt;
      if (it.value("value").isNull()) p.value = std::nullopt;
      else p.value = it.value("value").toDouble();

      out.push_back(p);
    }
  }

  std::sort(out.begin(), out.end(), [](const auto& a, const auto& b){ return a.dt < b.dt; });
  return out;
}

bool LocalDb::upsertSeries(
  int sensorId, const QString& paramCode, const QVector<MeasurementPoint>& pts, QString* err
) const {
  QFile f(m_path);
  if (!f.open(QIODevice::ReadOnly)) {
    if (err) *err = "cannot open db.json (read)";
    return false;
  }

  auto doc = readJsonDocument(f, err);
  f.close();

  if (!doc.isObject()) {
    if (err && err->isEmpty()) *err = "db.json is not object";
    return false;
  }

  auto root = doc.object();
  auto measurements = root.value("measurements").toArray();

  int idx = -1;
  for (int i = 0; i < measurements.size(); i++) {
    const auto o = measurements[i].toObject();
    if (o.value("sensorId").toInt() == sensorId) { idx = i; break; }
  }

  QJsonObject entry;
  if (idx >= 0) entry = measurements[idx].toObject();
  else {
    entry["sensorId"] = sensorId;
    entry["paramCode"] = paramCode;
    entry["items"] = QJsonArray{};
  }

  // Kluczem scalania jest data ISO, dzięki czemu kolejne zapisy nadpisują
  // tylko te punkty, które faktycznie się powtarzają.
  QMap<QString, QJsonObject> byDate;

  const auto existingItems = entry.value("items").toArray();
  for (const auto& v : existingItems) {
    const auto o = v.toObject();
    const auto k = o.value("date").toString();
    if (!k.isEmpty()) byDate[k] = o;
  }

  for (const auto& p : pts) {
    const QString k = p.dt.toString(Qt::ISODate);
    QJsonObject o;
    o["date"] = k;
    if (!p.value.has_value()) o["value"] = QJsonValue();
    else o["value"] = *p.value;
    byDate[k] = o;
  }

  QJsonArray merged;
  for (auto it = byDate.begin(); it != byDate.end(); ++it) {
    merged.append(it.value());
  }

  entry["items"] = merged;

  if (idx >= 0) measurements[idx] = entry;
  else measurements.append(entry);

  root["measurements"] = measurements;

  // QSaveFile zmniejsza ryzyko pozostawienia uszkodzonego pliku po niepełnym zapisie.
  QSaveFile outFile(m_path);
  if (!outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    if (err) *err = "cannot open db.json (write)";
    return false;
  }

  if (outFile.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0) {
    if (err) *err = "cannot write db.json";
    outFile.cancelWriting();
    return false;
  }

  if (!outFile.commit()) {
    if (err) *err = "cannot commit db.json";
    return false;
  }

  return true;
}
