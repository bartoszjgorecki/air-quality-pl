# Air Quality Monitor

## Opis skrócony

Air Quality Monitor to aplikacja desktopowa napisana w C++ i Qt, która pobiera dane jakości powietrza z API GIOS, prezentuje je w formie list, wykresu i mapy, a następnie pozwala zapisać wybrane serie w lokalnej bazie JSON. Interfejs został przygotowany tak, aby jedna aplikacja obsługiwała zarówno tryb online, jak i scenariusz awarii sieci lub niedostępności usługi.

## Główne moduły

- `AppController` koordynuje przepływ między interfejsem QML, klientem REST, bazą lokalną i analizą danych.
- `GiosClient` odpowiada za komunikację HTTP z API GIOS i uruchamianie parsowania JSON w tle.
- `GiosParsers` normalizuje różne warianty odpowiedzi JSON do jednego modelu używanego przez aplikację.
- `LocalDb` zapisuje i odczytuje historię pomiarów z lokalnego pliku `db.json`.
- `Analyzer` oblicza podstawowe statystyki serii pomiarowej.
- `qml/main.qml` zawiera główny interfejs z listami stacji i sensorów, wykresem oraz mapą.

## Obsługa wyjątków

Projekt ma jawnie zaimplementowaną obsługę błędów:

- błędy sieci i timeouty są raportowane przez `GiosClient::error`,
- błędne odpowiedzi JSON kończą się wyjątkiem `std::runtime_error` w parserach,
- `AppController` przechwytuje `std::exception` i awarie nieznanego typu przy pobieraniu danych, zapisie i odczycie historii,
- lokalna baza raportuje błędy otwarcia, parsowania, zapisu i zatwierdzenia pliku,
- interfejs przełącza się w tryb offline i informuje użytkownika o dostępności lokalnej historii.

## Wielowątkowość

Aplikacja pozostaje responsywna dzięki połączeniu asynchronicznych żądań i pracy w tle:

- zapytania HTTP są wykonywane nieblokująco przez Qt Network,
- parsowanie odpowiedzi JSON odbywa się poza głównym wątkiem przy użyciu `QtConcurrent`,
- kolorowanie mapy uruchamia wiele niezależnych żądań dla widocznych stacji i ignoruje przestarzałe wyniki przez token żądania,
- interakcje wykresu i mapy działają niezależnie od pobierania danych z sieci.

## Wzorce projektowe

- `Facade / Application Controller`: `AppController`
- `Repository / Data Source`: `LocalDb`, `GiosClient`
- `Observer`: sygnały, sloty i bindingi QML
- `Adapter`: `GiosParsers`
- kandydat na `Strategy`: osobne strategie wyszukiwania stacji dla wszystkich stacji, filtrowania tekstowego i promienia

## Pokrycie wymagań projektu

Zaimplementowane zostały:

- pełna lista stacji z REST,
- filtrowanie listy stacji po wpisanym tekście,
- wyszukiwanie w promieniu od wpisanego adresu,
- mapa z markerami stacji,
- opcjonalne kolorowanie mapy po wyborze rodzaju pomiaru,
- lista sensorów dla wskazanej stacji,
- pobieranie danych pomiarowych dla sensora,
- zapis do lokalnej bazy JSON,
- wykres i analiza podstawowych statystyk,
- konfiguracja Doxygen,
- testy jednostkowe.

Szczegółowy opis architektury i przebiegu działania znajduje się w pliku [docs/opis_projektu.md](opis_projektu.md).

## Podprojekty pośrednie

W katalogu `podprojekty` znajdują się dwie prostsze wersje aplikacji, pokazujące kolejne etapy rozwoju:

- `projekt1_przycisk` zawiera minimalne okno z jednym przyciskiem,
- `projekt2_stacje_dane` zawiera pobieranie stacji, sensorów i danych pomiarowych bez wykresu, mapy i bazy offline.
