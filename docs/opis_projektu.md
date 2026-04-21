# Detailed Project Description: Air Quality Monitor

## 1. Project Goal

Air Quality Monitor is a desktop application written in C++ with the Qt framework. Its main goal is to download, filter, and present air-quality data published by GIOS. The application supports the full workflow: choosing a monitoring station, selecting a sensor, viewing measurements on a chart, and saving the downloaded series to a local JSON database.

A second important goal of the project is resilience to external failures. Because the data comes from a public REST API, the application must assume that the network may be unavailable, requests may time out, JSON payloads may change shape, or parts of the service may be temporarily unavailable. For that reason the codebase is split into separate modules for networking, parsing, local persistence, and UI coordination. This separation reduces the risk that one failing element breaks the whole application.

## 2. General Runtime Flow

The user workflow follows a few clear steps. First, the application downloads the full list of monitoring stations in Poland. That list can then be narrowed in two ways: by a standard text filter or by searching for stations within a selected radius from a typed address. After a station is selected, the application downloads the sensors available at that station. The user then chooses a sensor, for example PM10 or NO2, selects how many recent days should be visible on the chart, and the application downloads the measurement series from the API and presents it in the chosen time window.

At the same time the application computes basic statistics for the series: minimum, maximum, average, the number of missing values, and a simple upward or downward trend. The user can also save the currently downloaded series to a local JSON file. This makes it possible to load previously saved history even when the online API is unavailable and still present a chart for the selected period.

## 3. Architecture and Module Split

The project is divided into several logical parts.

### 3.1. Model Layer

The `src/model` directory contains the core data structures. `MeasurementPoint` represents a single measurement with a timestamp and value, while `Stats` stores the result of the series analysis. This is the smallest layer in the project, but it is central because both the analyzer and the local database use these structures.

### 3.2. Analysis Layer

The `Analyzer` module computes statistics from a vector of measurement points. This is where minimum, maximum, average, the number of missing values, and the simplified trend are calculated. Keeping this logic in a dedicated class makes unit testing straightforward and prevents calculation code from leaking into the UI layer.

### 3.3. Local Storage Layer

`LocalDb` manages the local `db.json` file. It is responsible for creating an empty database on first launch, inserting or updating the series for the selected sensor, and loading history for a chosen time range. Writes are performed through `QSaveFile`, which reduces the risk of leaving a corrupted file after an incomplete save. This layer knows nothing about the user interface and only performs data operations.

### 3.4. Network and Parsing Layer

`GiosClient` is responsible for communication with the GIOS API. It sends HTTP requests, applies timeouts, and receives raw responses. That raw JSON is not passed directly to the rest of the application. Instead, the response is handed over to `GiosParsers`, which converts several possible payload variants into one normalized structure used by the rest of the program.

This separation matters for two reasons. First, the parser logic can be tested independently from network access. Second, if the API starts returning a slightly different JSON structure, the fix is usually isolated to one module instead of spreading through the entire application.

### 3.5. Application Layer

`AppController` acts as the central coordinator. It connects the QML interface with the REST client, the analyzer, and the local database. It decides when to download stations, when to clear the sensor list, when to save a series, and when to switch the application into offline mode. Because of this, QML stays focused on presentation instead of business logic.

### 3.6. Interface Layer

The main interface is implemented in `qml/main.qml`. It contains the left column with station and sensor lists and the right side with the chart, map, and data cards. The user can filter stations, run radius-based search, choose the chart range in days, switch between chart and map view, and load local history.

The chart is interactive: it supports mouse-wheel zoom, drag-based panning, and double-click reset. Clicking a single chart point also reveals its exact value and timestamp. The map is interactive as well and supports both dragging and zooming.

## 4. Exception Handling and Fault Tolerance

One of the key project requirements was resilience to failure scenarios. In practice this was implemented at several levels.

At the network level, `GiosClient` reports request errors, invalid responses, and timeouts. At the parsing level, malformed JSON results in a `std::runtime_error`, which helps distinguish transport problems from payload-shape problems. In `AppController`, all major download and save operations are wrapped in `try/catch` blocks, so a single failed action does not terminate the entire program.

The local database also reports readable error messages, for example when the file is corrupted or cannot be opened. If online download fails, the application switches to offline mode and informs the user whether local history already exists for the selected sensor. That way the user is not left without feedback and can fall back to previously saved data.

## 5. Multithreading and Responsiveness

The project uses asynchronous and multithreaded techniques so the interface does not freeze while handling larger API responses. HTTP requests are executed asynchronously through `QNetworkAccessManager`, which means the UI thread does not wait idly for the server.

In addition, raw JSON responses are not parsed on the main thread. They are parsed in a worker thread through `QtConcurrent`. This is especially important for large station lists and measurement payloads. Because of that, the window, chart, and map remain responsive even while the application processes data in the background.

The map overlay adds one more mechanism: refreshing station colors can trigger many parallel requests together with a request token. If the user quickly changes the filter or selects another sensor, stale responses are ignored and cannot overwrite the newest application state.

## 6. Unit Tests

The project includes unit tests implemented with Qt Test. Three main areas are covered: the statistics analyzer, the local database, and the GIOS response parsers. Parser tests are particularly important because the API is not always uniform and can return different field names in practice. These tests make it easy to verify that the application still recognizes stations, sensors, and measurements after a payload-shape change.

The local database tests verify writes, updates, history reads, and handling of a corrupted JSON file. The analyzer tests verify the correctness of average, minimum, maximum, trend, and coverage calculations. This set does not cover the graphical interface directly, but it gives strong protection to the most important application logic.

## 7. Design Patterns

Several design patterns can be identified in the project. `AppController` acts as a `Facade` or `Application Controller` because it centralizes the coordination of operations visible from the GUI. `GiosClient` and `LocalDb` serve as `Repository`-like data sources. Qt signals and slots form a natural `Observer` mechanism because the interface reacts to state changes in C++ objects. `GiosParsers` can be treated as an `Adapter`, because it adapts the external JSON format to the program’s internal model.

## 8. Summary

The main strength of the project is that it combines several engineering concerns in one coherent application: REST API communication, asynchronous execution, basic data processing, local persistence, and a graphical interface. At the same time, the code is split into modules that can be developed and tested independently. Because of that, the application is not only a one-off demonstration, but a structured project that can be extended further.
