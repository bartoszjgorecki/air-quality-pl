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

// Zwraca pusty szablon bazy, który zapisujemy przy pierwszym uruchomieniu.
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

// Odczytuje i parsuje cały plik JSON, zapisując komunikat błędu do err.
static QJsonDocument readJsonDocument(QFile& file, QString* err) {
  QJsonParseError parseError;
  const auto doc = QJsonDocument::fromJson(file.readAll(), &parseError);
  if (parseError.error != QJsonParseError::NoError) {
    if (err) *err = "db.json parse error: " + parseError.errorString();
    return {};
  }
  return doc;
}

// Zapisuje cały obiekt root do pliku bazy w sposób możliwie bezpieczny.
static bool writeRootObject(const QString& path, const QJsonObject& root, QString* err) {
  // QSaveFile zmniejsza ryzyko pozostawienia uszkodzonego pliku po niepełnym zapisie.
  QSaveFile outFile(path);
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

// Zamienia listę QVariantMap na tablicę JSON, zachowując tylko sensowne rekordy.
static QJsonArray variantListToJsonArray(const QVariantList& list) {
  QJsonArray array;
  QSet<int> seenIds;

  for (const auto& value : list) {
    const QVariantMap map = value.toMap();
    if (map.isEmpty()) continue;

    const int id = map.value("id").toInt();
    if (id > 0) {
      if (seenIds.contains(id)) continue;
      seenIds.insert(id);
    }

    array.append(QJsonObject::fromVariantMap(map));
  }

  return array;
}

// Zamienia tablicę JSON z prostymi obiektami na listę używaną przez QML.
static QVariantList jsonArrayToVariantList(const QJsonArray& array) {
  QVariantList list;
  list.reserve(array.size());

  for (const auto& value : array) {
    if (!value.isObject()) continue;
    list.push_back(value.toObject().toVariantMap());
  }

  return list;
}

// Zapamiętuje ścieżkę do pliku pełniącego rolę lokalnej bazy.
LocalDb::LocalDb(QString path) : m_path(std::move(path)) {}

// Gwarantuje istnienie pliku db.json jeszcze przed pierwszym zapisem.
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

// Wczytuje ostatnio zapisaną listę stacji, aby interfejs mógł działać także offline.
QVariantList LocalDb::loadStations(QString* err) const {
  QVariantList out;
  if (err) err->clear();

  QString ensureErr;
  if (!ensureExists(&ensureErr)) {
    if (err) *err = ensureErr;
    return out;
  }

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

  return jsonArrayToVariantList(doc.object().value("stations").toArray());
}

// Podmienia pełną zapisaną listę stacji na najnowszą migawkę pobraną online.
bool LocalDb::replaceStations(const QVariantList& stations, QString* err) const {
  if (err) err->clear();

  QString ensureErr;
  if (!ensureExists(&ensureErr)) {
    if (err) *err = ensureErr;
    return false;
  }

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
  root["stations"] = variantListToJsonArray(stations);
  return writeRootObject(m_path, root, err);
}

// Wczytuje zapisane sensory dla jednej stacji, jeśli były już wcześniej pobrane.
QVariantList LocalDb::loadSensorsForStation(int stationId, QString* err) const {
  QVariantList out;
  if (err) err->clear();

  QString ensureErr;
  if (!ensureExists(&ensureErr)) {
    if (err) *err = ensureErr;
    return out;
  }

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

  const auto sensors = doc.object().value("sensors").toArray();
  for (const auto& value : sensors) {
    const auto entry = value.toObject();
    if (entry.value("stationId").toInt() != stationId) continue;
    return jsonArrayToVariantList(entry.value("items").toArray());
  }

  if (err) *err = "no cached sensors for station";
  return out;
}

// Aktualizuje zapisane sensory tylko dla wskazanej stacji, bez ruszania innych wpisów.
bool LocalDb::upsertSensorsForStation(int stationId, const QVariantList& sensors, QString* err) const {
  if (err) err->clear();

  QString ensureErr;
  if (!ensureExists(&ensureErr)) {
    if (err) *err = ensureErr;
    return false;
  }

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
  auto storedSensors = root.value("sensors").toArray();

  int idx = -1;
  for (int i = 0; i < storedSensors.size(); i++) {
    const auto entry = storedSensors[i].toObject();
    if (entry.value("stationId").toInt() == stationId) {
      idx = i;
      break;
    }
  }

  QJsonObject entry;
  entry["stationId"] = stationId;
  entry["items"] = variantListToJsonArray(sensors);

  if (idx >= 0) storedSensors[idx] = entry;
  else storedSensors.append(entry);

  root["sensors"] = storedSensors;
  return writeRootObject(m_path, root, err);
}

// Sprawdza tylko obecność serii, bez wczytywania całych pomiarów do UI.
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

// Odczytuje punkty pomiarowe dla jednego sensora z podanego zakresu czasu.
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

  // W jednym pliku może być wiele sensorów, więc najpierw wybieramy tylko ten właściwy.
  // Szukamy tylko wpisów odpowiadających właściwemu sensorowi.
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

  // Sortowanie gwarantuje, że wykres i analiza dostaną punkty w kolejności czasu.
  std::sort(out.begin(), out.end(), [](const auto& a, const auto& b){ return a.dt < b.dt; });
  return out;
}

// Odczytuje całą historię sensora bez zawężania do zadanego zakresu czasu.
QVector<MeasurementPoint> LocalDb::loadAllHistory(int sensorId, QString* err) const {
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

  const auto measurements = doc.object().value("measurements").toArray();
  for (const auto& entryVal : measurements) {
    const auto entry = entryVal.toObject();
    if (entry.value("sensorId").toInt() != sensorId) continue;

    const auto items = entry.value("items").toArray();
    for (const auto& itVal : items) {
      const auto it = itVal.toObject();
      const auto dt = QDateTime::fromString(it.value("date").toString(), Qt::ISODate);
      if (!dt.isValid()) continue;

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

// Wstawia nową serię lub scala ją z już istniejącą dla tego samego sensora.
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

  // Szukamy, czy dla tego sensora istnieje już zapisany wpis.
  int idx = -1;
  for (int i = 0; i < measurements.size(); i++) {
    const auto o = measurements[i].toObject();
    if (o.value("sensorId").toInt() == sensorId) { idx = i; break; }
  }

  QJsonObject entry;
  if (idx >= 0) entry = measurements[idx].toObject();
  else {
    // Przy pierwszym zapisie tworzymy nowy kontener dla sensora.
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

  // Po scaleniu podmieniamy starą zawartość wpisu jedną, uporządkowaną tablicą.
  entry["items"] = merged;

  if (idx >= 0) measurements[idx] = entry;
  else measurements.append(entry);

  root["measurements"] = measurements;
  return writeRootObject(m_path, root, err);
}
