# air-quality-pl
JPO Project 2026

# Air Quality Monitor

Air Quality Monitor is a desktop C++/Qt application for browsing Polish air-quality stations, downloading measurements from the GIOS REST API, saving them in a local JSON database, and presenting both charts and map-based views.

## Implemented Features

- download the full list of monitoring stations from the GIOS API
- filter stations by city, station name, or address
- find stations within a chosen radius from a typed address
- display stations on a map
- color map markers by the currently selected measurement type
- download sensors for the selected station
- download measurements for the selected sensor
- save downloaded series to a local JSON database
- fall back to saved local history when the API is unavailable
- display a chart with min, max, average, trend, and timestamps for min/max values
- run unit tests for parser, storage, and analysis logic

## Requirements

- CMake 3.21+
- C++17 compiler
- Qt 6 with modules: `Core`, `Gui`, `Widgets`, `Qml`, `Quick`, `QuickControls2`, `Network`, `Positioning`, `Location`, `Charts`, `Multimedia`, `Test`, `Concurrent`

## Build

```bash
cmake -S . -B build-cmake
cmake --build build-cmake
```

## Run

```bash
./build-cmake/air_quality_pl
```

## Usage Guide

1. Click `Download` to load monitoring stations from the GIOS API.
2. Use `Filter by city` to narrow the station list locally.
3. Optionally type an address in `Address for radius search`, choose `Radius (km)`, and click `Locate`.
4. Switch to the `Map` tab if you want to inspect stations spatially.
5. Click a station in the list or on the map to load its available sensors.
6. Click a sensor to download its measurements and refresh the chart.
7. Review the `Analysis` card for minimum, maximum, average, trend, and timestamps.
8. Click `Save to Local DB` to persist the current series in the local JSON database.
9. Use `History range` and `Load Local History` to display previously saved data when needed.

## What The Map Status Means

If the map shows a message such as `No visible stations provide NO measurements.`, it means:

- a sensor is selected, so the app is looking for the same pollutant across the currently visible stations
- none of the stations visible after the current filters expose that measurement type
- changing the station filter, the radius filter, or the selected sensor may produce colored markers again

## Exception Handling

The application is designed to stay usable when the API or local storage fails:

- network failures, timeouts, and malformed JSON are surfaced to the UI through explicit error messages
- parser helpers throw `std::runtime_error` on invalid payload shapes
- `AppController` catches `std::exception` and fallback unknown exceptions around station, sensor, measurement, and local DB actions
- the local JSON database reports read and write errors through user-facing status text
- if online access fails and local history exists, the UI informs the user that saved data can still be loaded

## Multithreading

The application uses asynchronous and multithreaded processing in two places:

- HTTP requests are asynchronous through Qt networking
- raw JSON decoding runs on a worker thread with `QtConcurrent`, so large API responses do not block the UI thread
- map coloring fans out multiple asynchronous station/sensor/measurement requests and discards stale batches with a request token

## Design Patterns Used

- `Facade / Application Controller`: `AppController` exposes one UI-facing coordination layer
- `Repository / Data Source`: `LocalDb` handles persisted JSON data and `GiosClient` handles remote REST access
- `Observer`: Qt signals, slots, and QML bindings propagate data changes reactively
- `Adapter`: `GiosParsers` adapts unstable GIOS payload formats into the app’s internal model
- `Strategy` candidate: station search modes can be treated as separate strategies as the project grows

## Project Status Against The Assignment

The current codebase covers the core functional requirements from the assignment:

- full station list from REST
- filtered station list by user-provided text
- radius-based station search from a typed location
- map with station markers
- optional color coding on the map after selecting a measurement type
- sensor list for the selected station
- measurement download for the selected sensor
- local JSON database persistence
- chart presentation
- basic analysis
- GUI
- Doxygen configuration
- unit tests

Items that depend on the environment rather than only on code:

- GitHub publication cannot be verified from this workspace
- cross-platform support is based on Qt/CMake design; the current workspace was verified locally, not on every target OS

## Known Limitations

- map color coding is intentionally limited to at most `40` visible stations at once to avoid excessive API fan-out
- address geocoding and map tiles depend on online access to the configured Qt Location provider
- pollutant labels are normalized to English for the UI, but raw GIOS payloads may still contain localized source names in debug dumps
- Doxygen configuration is included, but `doxygen` is not bundled with the project

## Run Tests

```bash
ctest --test-dir build-cmake --output-on-failure
```

You can also run the test binary directly:

```bash
./build-cmake/air_quality_pl_tests
```

## Generate Documentation

If Doxygen is installed:

```bash
doxygen Doxyfile
```

The generated HTML documentation is written to `docs/html`.
