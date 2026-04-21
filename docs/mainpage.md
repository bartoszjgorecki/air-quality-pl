# Air Quality Monitor

## Short Description

Air Quality Monitor is a desktop C++/Qt application that downloads air-quality data from the GIOS API, presents it as lists, charts, and map overlays, and allows selected measurement series to be saved in a local JSON database. The interface is designed so that one application can handle both the online workflow and the offline scenario when the network or service is unavailable.

## Main Modules

- `AppController` coordinates the flow between the QML interface, the REST client, the local database, and the analysis layer.
- `GiosClient` handles HTTP communication with the GIOS API and starts JSON parsing in the background.
- `GiosParsers` normalizes multiple JSON response variants into one model used by the rest of the application.
- `LocalDb` stores and loads measurement history from the local `db.json` file.
- `Analyzer` computes basic statistics for a measurement series.
- `qml/main.qml` contains the main interface with station and sensor lists, the chart, and the map.

## Exception Handling

The project includes explicit error handling:

- network errors and timeouts are reported through `GiosClient::error`,
- malformed JSON responses throw `std::runtime_error` in the parser layer,
- `AppController` catches `std::exception` and unknown failures around downloads, saves, and history reads,
- the local database reports file open, parse, save, and commit errors,
- the interface switches to offline mode and informs the user whether local history is available.

## Multithreading

The application stays responsive thanks to a combination of asynchronous requests and background work:

- HTTP requests are executed asynchronously through Qt Network,
- JSON parsing runs off the main thread with `QtConcurrent`,
- map coloring fans out multiple independent requests for visible stations and ignores stale results through a request token,
- chart and map interactions remain independent from background network activity.

## Design Patterns

- `Facade / Application Controller`: `AppController`
- `Repository / Data Source`: `LocalDb`, `GiosClient`
- `Observer`: Qt signals, slots, and QML bindings
- `Adapter`: `GiosParsers`
- `Strategy` candidate: separate station-search strategies for the full list, text filtering, and radius-based lookup

## Assignment Coverage

Implemented features include:

- full station list from REST,
- station filtering by typed text,
- radius search from a typed address,
- a map with station markers,
- optional map coloring after selecting a measurement type,
- a sensor list for the selected station,
- measurement download for the selected sensor,
- selection of the most recent days shown on the chart for both online data and local history,
- persistence to a local JSON database,
- chart presentation and basic statistical analysis,
- Doxygen configuration,
- unit tests.

A more detailed description of the architecture and runtime flow is available in [docs/opis_projektu.md](opis_projektu.md).
