# Air Quality Monitor

## Overview

Air Quality Monitor is a Qt-based desktop application that monitors Polish air-quality data published by the GIOS REST API. The application lets the user browse stations, inspect sensors, download measurements, save local history in JSON format, display charts, and color station markers on the map by the selected pollutant.

## Main Modules

- `AppController`: UI-facing coordination layer for online data, offline history, statistics, and map overlays
- `GiosClient`: asynchronous HTTP client for the remote API
- `GiosParsers`: payload adapters that normalize localized and legacy GIOS JSON payloads
- `LocalDb`: JSON-backed local persistence layer
- `Analyzer`: basic statistical analysis for measurement series
- `qml/main.qml`: main graphical interface, chart view, station list, and map view

## Map Overlay

When a sensor is selected, the map can color visible stations by the latest available value of the same pollutant. The legend uses a blue-to-red scale:

- blue and cyan for lower values
- green and yellow for middle values
- orange and red for higher values

## Chart Interaction

The chart view supports interactive inspection of downloaded series:

- mouse wheel zoom
- drag to pan
- double-click to reset the default view
- click on a visible point to inspect its exact value and timestamp

## Exception Handling

The project includes explicit error handling for the main failure scenarios:

- network requests report failures through `GiosClient::error`
- malformed JSON payloads are rejected by `GiosParsers` with `std::runtime_error`
- `AppController` catches `std::exception` and fallback unknown exceptions around online and local-data actions
- the local JSON database reports parse, open, write, and commit failures
- the UI switches to offline mode and informs the user when saved local history is available

## Multithreading And Responsiveness

The application uses both asynchronous operations and background work:

- Qt network requests are non-blocking
- JSON parsing is moved to a worker thread with `QtConcurrent`
- map coloring dispatches multiple asynchronous station and measurement lookups and uses a request token to ignore stale responses
- chart inspection remains responsive because it is decoupled from the asynchronous data-loading pipeline

This keeps the QML/UI thread responsive while larger API responses are being processed.

## Design Patterns

- `Facade / Application Controller`: `AppController`
- `Repository / Data Source`: `LocalDb`, `GiosClient`
- `Observer`: Qt signals, slots, and QML property bindings
- `Adapter`: `GiosParsers`
- `Strategy` candidate for future growth: separate station-search strategies for all stations, city filter, and radius search

## Assignment Coverage

The current implementation covers the following assignment points:

- full station list from REST
- filtered station list by typed search
- radius search from a typed address
- map with station markers
- optional map color coding after sensor selection
- sensor list for the selected station
- measurement download for the selected sensor
- local database in JSON format
- chart presentation
- basic analysis
- GUI
- Doxygen setup
- unit tests

The following points are outside what can be fully verified from the current workspace alone:

- publication in GitHub
- runtime verification on every target platform

## Known Limitations

- map color coding is limited to `40` visible stations per refresh to keep the number of API requests under control
- geocoding and map tiles require online access to the active Qt Location provider
- some raw source payloads remain localized in debug files even though the visible UI now uses English labels
- HTML documentation generation depends on a locally installed `doxygen` binary

## Running

```bash
cmake -S . -B build-cmake
cmake --build build-cmake
./build-cmake/air_quality_pl
```

## Running Tests

```bash
ctest --test-dir build-cmake --output-on-failure
```
