#include "GiosParsers.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>
#include <QUrlQuery>

#include <algorithm>
#include <stdexcept>

namespace {

// Funkcje pomocnicze poniżej pozwalają czytać JSON "luźno", bo publiczne API GIOS
// potrafi zmieniać nazwy pól, typy wartości albo zagnieżdżenie obiektów.

// Czyta wartość tekstową nawet wtedy, gdy API poda ją jako liczbę, obiekt albo tablicę.
QString readString(const QJsonValue& v) {
  // API GIOS nie zawsze zwraca ten sam typ wartości, więc czytamy dość liberalnie.
  if (v.isString()) return v.toString();
  if (v.isDouble()) return QString::number(v.toDouble());
  if (v.isObject()) {
    const auto o = v.toObject();
    if (o.contains("@value")) return o.value("@value").toVariant().toString();
    if (o.contains("value")) return o.value("value").toVariant().toString();
  }
  if (v.isArray()) {
    const auto a = v.toArray();
    for (const auto& x : a) {
      const auto s = readString(x);
      if (!s.isEmpty()) return s;
    }
  }
  return {};
}

// Odczyt liczby całkowitej ze struktur o różnym kształcie.
int readInt(const QJsonValue& v) {
  if (v.isDouble()) return static_cast<int>(v.toDouble());
  bool ok = false;
  const int n = readString(v).toInt(&ok);
  return ok ? n : 0;
}

// Odczyt liczby zmiennoprzecinkowej ze struktur o różnym kształcie.
double readDouble(const QJsonValue& v) {
  if (v.isDouble()) return v.toDouble();
  bool ok = false;
  const double d = readString(v).toDouble(&ok);
  return ok ? d : 0.0;
}

// Szuka tablicy po fragmencie nazwy pola, co pomaga przy lokalizowanych payloadach.
QJsonArray findArrayByKeyContains(const QJsonObject& o, const QString& needle) {
  for (auto it = o.begin(); it != o.end(); ++it) {
    if (it.key().contains(needle, Qt::CaseInsensitive) && it.value().isArray()) {
      return it.value().toArray();
    }
  }
  return {};
}

// Próbuje znaleźć tablicę pod jedną z oczekiwanych nazw kluczy.
bool tryGetArray(const QJsonObject& o, const QStringList& keys, QJsonArray* out) {
  for (const auto& key : keys) {
    const auto value = o.value(key);
    if (value.isArray()) {
      if (out) *out = value.toArray();
      return true;
    }
  }
  return false;
}

// GIOS czasem chowa obiekty w tablicach, więc wyciągamy pierwszy sensowny obiekt.
QJsonObject asObjectLoose(const QJsonValue& v) {
  if (v.isObject()) return v.toObject();
  if (v.isArray()) {
    const auto a = v.toArray();
    for (const auto& x : a) {
      if (x.isObject()) return x.toObject();
    }
  }
  return {};
}

// Jedno miejsce do zgłaszania błędów parsowania jako std::runtime_error.
[[noreturn]] void throwParseError(const QString& message) {
  throw std::runtime_error(message.toStdString());
}

// Mapuje najczęstsze nazwy parametrów z polskiego API na angielskie etykiety UI.
QString translatedParamName(const QString& rawName, const QString& paramCode) {
  // UI działa po angielsku, więc normalizujemy najczęstsze polskie nazwy.
  const QString normalizedCode = paramCode.trimmed().toUpper();

  if (normalizedCode == "NO") return "Nitric oxide";
  if (normalizedCode == "NO2") return "Nitrogen dioxide";
  if (normalizedCode == "NOX") return "Nitrogen oxides";
  if (normalizedCode == "O3") return "Ozone";
  if (normalizedCode == "SO2") return "Sulfur dioxide";
  if (normalizedCode == "CO") return "Carbon monoxide";
  if (normalizedCode == "C6H6") return "Benzene";
  if (normalizedCode == "PM10") return "Particulate matter PM10";
  if (normalizedCode == "PM2.5" || normalizedCode == "PM25") return "Particulate matter PM2.5";
  if (normalizedCode == "BAP" || normalizedCode == "BAP(PM10)") return "Benzo(a)pyrene";
  if (normalizedCode == "AS") return "Arsenic";
  if (normalizedCode == "CD") return "Cadmium";
  if (normalizedCode == "NI") return "Nickel";
  if (normalizedCode == "PB") return "Lead";

  const QString normalizedRaw = rawName.trimmed().toLower();
  if (normalizedRaw == "tlenek azotu") return "Nitric oxide";
  if (normalizedRaw == "dwutlenek azotu") return "Nitrogen dioxide";
  if (normalizedRaw == "tlenki azotu") return "Nitrogen oxides";
  if (normalizedRaw == "ozon") return "Ozone";
  if (normalizedRaw == "dwutlenek siarki") return "Sulfur dioxide";
  if (normalizedRaw == "tlenek węgla" || normalizedRaw == "tlenek wegla") return "Carbon monoxide";
  if (normalizedRaw == "benzen") return "Benzene";
  if (normalizedRaw == "pył zawieszony pm10" || normalizedRaw == "pyl zawieszony pm10") return "Particulate matter PM10";
  if (normalizedRaw == "pył zawieszony pm2.5" || normalizedRaw == "pyl zawieszony pm2.5") return "Particulate matter PM2.5";
  if (normalizedRaw == "benzo(a)piren" || normalizedRaw == "benzo(a)piren w pm10") return "Benzo(a)pyrene";
  if (normalizedRaw == "arsen") return "Arsenic";
  if (normalizedRaw == "kadm") return "Cadmium";
  if (normalizedRaw == "nikiel") return "Nickel";
  if (normalizedRaw == "ołów" || normalizedRaw == "olow") return "Lead";

  return rawName.trimmed();
}

}  // namespace

namespace GiosParsers {

// Parsuje jedną stronę odpowiedzi stacji i rozpoznaje, czy paginacja dobiegła końca.
StationsPage parseStationsPage(const QJsonDocument& doc, const QUrl& currentUrl) {
  // Dla stacji akceptujemy kilka wariantów kluczy, bo endpoint bywał zmieniany.
  if (!doc.isObject()) {
    throwParseError("Stations payload is not a JSON object");
  }

  const auto root = doc.object();
  QJsonArray arr;
  // Obsługujemy zarówno aktualny JSON, jak i starsze warianty nazw pól.
  const bool hasExplicitArray =
    tryGetArray(root, {"Lista stacji pomiarowych", "stations", "stationList"}, &arr);
  if (!hasExplicitArray) {
    arr = findArrayByKeyContains(root, "Lista stacji");
    if (arr.isEmpty()) {
      throwParseError("Stations payload does not contain a stations array");
    }
  }

  StationsPage page;
  page.stations.reserve(arr.size());

  for (const auto& v : arr) {
    const auto o = v.toObject();

    // Identyfikator jest kluczowy, więc rekordy bez poprawnego ID są pomijane.
    const int idPl = readInt(o.value("Identyfikator stacji"));
    const int idEn = readInt(o.value("id"));
    const int id = idPl != 0 ? idPl : idEn;
    if (id == 0) continue;

    QVariantMap m;
    m["id"] = id;

    const QString name = !readString(o.value("Nazwa stacji")).isEmpty()
      ? readString(o.value("Nazwa stacji"))
      : readString(o.value("stationName"));
    m["name"] = name;

    const double lat = o.contains("WGS84 φ N") ? readDouble(o.value("WGS84 φ N")) : readDouble(o.value("gegrLat"));
    const double lon = o.contains("WGS84 λ E") ? readDouble(o.value("WGS84 λ E")) : readDouble(o.value("gegrLon"));
    m["lat"] = lat;
    m["lon"] = lon;

    const QString city = !readString(o.value("Nazwa miasta")).isEmpty()
      ? readString(o.value("Nazwa miasta"))
      : readString(asObjectLoose(o.value("city")).value("name"));
    m["city"] = city;

    const QString province = !readString(o.value("Województwo")).isEmpty()
      ? readString(o.value("Województwo"))
      : readString(asObjectLoose(asObjectLoose(o.value("city")).value("commune")).value("provinceName"));
    m["province"] = province;

    const QString district = !readString(o.value("Powiat")).isEmpty()
      ? readString(o.value("Powiat"))
      : readString(asObjectLoose(asObjectLoose(o.value("city")).value("commune")).value("districtName"));
    m["district"] = district;

    const QString address = o.contains("Ulica") ? readString(o.value("Ulica")) : readString(o.value("addressStreet"));
    m["address"] = address;

    page.stations.push_back(m);
  }

  const auto links = root.value("links").toObject();
  page.nextUrl = readString(links.value("next"));

  const QString selfUrl = readString(links.value("self"));
  const int totalPages = root.value("totalPages").toInt();
  const int currentPage = QUrlQuery(currentUrl).queryItemValue("page").toInt();

  // API czasem zwraca pustą stronę końcową albo zapętlone "next".
  const bool reachedLastPage = totalPages > 0 && currentPage >= totalPages - 1;
  const bool emptyTrailingPage = page.stations.isEmpty() && currentPage > 0;
  const bool loopingNext = !page.nextUrl.isEmpty() && page.nextUrl == selfUrl;
  page.reachedEnd = page.nextUrl.isEmpty() || reachedLastPage || emptyTrailingPage || loopingNext;

  return page;
}

// Parsuje listę sensorów zarówno z nowszego, jak i starszego wariantu odpowiedzi.
QVariantList parseSensors(const QJsonDocument& doc) {
  QVariantList out;

  // Nowsza wersja endpointu zwraca od razu tablicę obiektów.
  if (doc.isArray()) {
    const auto arr = doc.array();
    out.reserve(arr.size());
    for (const auto& v : arr) {
      const auto o = v.toObject();
      const auto p = o.value("param").toObject();

      // Wariant tablicowy odpowiada nowszej strukturze endpointu sensorów.
      QVariantMap m;
      m["id"] = readInt(o.value("id"));
      m["stationId"] = readInt(o.value("stationId"));
      const QString paramName = readString(p.value("paramName"));
      const QString paramCode = readString(p.value("paramCode"));
      m["paramName"] = paramName;
      m["paramCode"] = paramCode;
      m["displayName"] = translatedParamName(paramName, paramCode);
      m["paramFormula"] = readString(p.value("paramFormula"));
      m["idParam"] = readInt(p.value("idParam"));
      out.push_back(m);
    }
    return out;
  }

  if (!doc.isObject()) {
    throwParseError("Sensors payload is neither an array nor an object");
  }

  const auto root = doc.object();
  QJsonArray arr;
  // Starsze lub lokalizowane warianty odpowiedzi mają różne nazwy kluczy.
  const bool hasExplicitArray =
    tryGetArray(root, {"Lista stanowisk pomiarowych dla podanej stacji", "stations", "sensors"}, &arr);
  if (!hasExplicitArray) {
    arr = findArrayByKeyContains(root, "Lista stanowisk");
  }
  if (arr.isEmpty()) {
    arr = findArrayByKeyContains(root, "Lista czuj");
  }
  if (arr.isEmpty()) {
    throwParseError("Sensors payload does not contain a sensors array");
  }

  out.reserve(arr.size());
  for (const auto& v : arr) {
    const auto o = v.toObject();

    // Wariant obiektowy pozwala obsłużyć też starsze, lokalizowane nazwy pól.
    QVariantMap m;
    m["id"] = o.contains("Identyfikator stanowiska") ? readInt(o.value("Identyfikator stanowiska")) : readInt(o.value("id"));
    m["stationId"] = o.contains("Identyfikator stacji") ? readInt(o.value("Identyfikator stacji")) : readInt(o.value("stationId"));

    const QString paramName = o.contains("Wskaźnik") ? readString(o.value("Wskaźnik"))
                              : (o.contains("Nazwa parametru") ? readString(o.value("Nazwa parametru")) : readString(o.value("paramName")));
    const QString paramCode = o.contains("Wskaźnik - kod") ? readString(o.value("Wskaźnik - kod"))
                              : (o.contains("Kod parametru") ? readString(o.value("Kod parametru")) : readString(o.value("paramCode")));
    m["paramName"] = paramName;
    m["paramCode"] = paramCode;
    m["displayName"] = translatedParamName(paramName, paramCode);
    m["paramFormula"] = o.contains("Wskaźnik - wzór") ? readString(o.value("Wskaźnik - wzór"))
                         : (o.contains("Symbol parametru") ? readString(o.value("Symbol parametru")) : readString(o.value("paramFormula")));
    m["idParam"] = o.contains("Id wskaźnika") ? readInt(o.value("Id wskaźnika"))
                   : (o.contains("Identyfikator parametru") ? readInt(o.value("Identyfikator parametru")) : readInt(o.value("idParam")));
    out.push_back(m);
  }

  return out;
}

// Parsuje serię pomiarową, filtruje błędne rekordy i sortuje punkty po czasie.
MeasurementSeries parseMeasurements(const QJsonDocument& doc) {
  // W pomiarach najważniejsze jest uporządkowanie punktów po czasie
  // i odrzucenie rekordów, których nie da się wiarygodnie użyć.
  if (!doc.isObject()) {
    throwParseError("Measurements payload is not a JSON object");
  }

  const auto root = doc.object();
  MeasurementSeries series;
  series.paramCode = readString(root.value("key"));

  QJsonArray arr = root.value("values").toArray();
  if (arr.isEmpty()) {
    tryGetArray(root, {"Lista danych pomiarowych", "values", "measurements"}, &arr);
  }
  if (arr.isEmpty() && !root.contains("values") && !root.contains("Lista danych pomiarowych")) {
    throwParseError("Measurements payload does not contain a values array");
  }

  struct ParsedPoint {
    qint64 t = 0;
    double v = 0.0;
  };

  // Najpierw zbieramy dane do bufora pomocniczego, potem sortujemy po czasie.
  QVector<ParsedPoint> parsed;
  parsed.reserve(arr.size());

  for (const auto& v : arr) {
    const auto it = v.toObject();
    const auto dateStr = it.contains("Data") ? readString(it.value("Data")) : readString(it.value("date"));
    // Rekord bez poprawnej daty nic nie daje na wykresie, więc go pomijamy.
    if (dateStr.isEmpty()) continue;

    QDateTime dt = QDateTime::fromString(dateStr, "yyyy-MM-dd HH:mm:ss");
    if (!dt.isValid()) dt = QDateTime::fromString(dateStr, Qt::ISODate);
    if (!dt.isValid()) continue;

    const auto valueNode = it.contains("Wartość") ? it.value("Wartość") : it.value("value");
    // Brak wartości traktujemy jako punkt bezużyteczny dla bieżącej wizualizacji.
    if (valueNode.isNull()) continue;
    const double val = readDouble(valueNode);

    if (series.paramCode.isEmpty() && it.contains("Kod stanowiska")) {
      series.paramCode = readString(it.value("Kod stanowiska"));
    }

    parsed.push_back({dt.toMSecsSinceEpoch(), val});
  }

  // Sortowanie gwarantuje poprawny porządek na wykresie nawet przy niespójnej odpowiedzi API.
  std::sort(parsed.begin(), parsed.end(), [](const ParsedPoint& a, const ParsedPoint& b) {
    return a.t < b.t;
  });

  series.points.reserve(parsed.size());
  for (const auto& point : parsed) {
    QVariantMap p;
    p["t"] = point.t;
    p["v"] = point.v;
    series.points.push_back(p);
  }

  return series;
}

}  // namespace GiosParsers
