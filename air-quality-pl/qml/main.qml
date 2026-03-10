import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtCharts 2.15
import QtLocation 6.10
import QtPositioning 6.10
import "ui"

Window {
  visible: true
  width: 1280
  height: 820
  minimumWidth: 1100
  minimumHeight: 800
  title: "Air Quality Monitor"
  property int selectedStationId: -1
  property var filteredStations: []
  property var defaultMapCenter: QtPositioning.coordinate(52.0693, 19.4803)
  property var radiusSearchCenter: QtPositioning.coordinate()
  property string radiusSearchAddress: ""
  property bool radiusFilterEnabled: false
  property string radiusSearchMessage: ""
  property var currentMapCenter: defaultMapCenter
  property real currentMapZoomLevel: 6.2
  property string addressLookupMessage: ""

  function hasValidCoordinate(coordinate) {
    return coordinate && coordinate.isValid
  }

  function stationCoordinate(station) {
    if (!station)
      return QtPositioning.coordinate()
    var lat = Number(station.lat)
    var lon = Number(station.lon)
    if (!isFinite(lat) || !isFinite(lon))
      return QtPositioning.coordinate()
    var coordinate = QtPositioning.coordinate(lat, lon)
    return coordinate.isValid ? coordinate : QtPositioning.coordinate()
  }

  function stationDistanceKm(station) {
    if (!radiusFilterEnabled || !hasValidCoordinate(radiusSearchCenter))
      return null
    var coordinate = stationCoordinate(station)
    if (!hasValidCoordinate(coordinate))
      return null
    return coordinate.distanceTo(radiusSearchCenter) / 1000.0
  }

  function selectedStationObject() {
    var stations = App.stations || []
    for (var i = 0; i < stations.length; i++) {
      if (stations[i].id === selectedStationId)
        return stations[i]
    }
    return null
  }

  function updateMapViewState() {
    var selectedStation = selectedStationObject()
    var selectedCoordinate = stationCoordinate(selectedStation)

    if (radiusFilterEnabled && hasValidCoordinate(radiusSearchCenter)) {
      currentMapCenter = radiusSearchCenter
      var radius = radiusKm.value
      if (radius <= 5) currentMapZoomLevel = 12.8
      else if (radius <= 15) currentMapZoomLevel = 11.2
      else if (radius <= 30) currentMapZoomLevel = 10.2
      else if (radius <= 60) currentMapZoomLevel = 9.1
      else currentMapZoomLevel = 8.1
      return
    }

    if (hasValidCoordinate(selectedCoordinate)) {
      currentMapCenter = selectedCoordinate
      currentMapZoomLevel = 11.5
      return
    }

    if (filteredStations.length > 0) {
      var firstCoordinate = stationCoordinate(filteredStations[0])
      if (hasValidCoordinate(firstCoordinate)) {
        currentMapCenter = firstCoordinate
        currentMapZoomLevel = 7.0
        return
      }
    }

    currentMapCenter = defaultMapCenter
    currentMapZoomLevel = 6.2
  }

  function clearRadiusSearch() {
    radiusSearchCenter = QtPositioning.coordinate()
    radiusSearchAddress = ""
    radiusFilterEnabled = false
    radiusSearchMessage = ""
    addressLookupMessage = ""
    geocodeModel.query = ""
    updateFilteredStations()
  }

  function currentFilteredStationIds() {
    var ids = []
    for (var i = 0; i < filteredStations.length; i++)
      ids.push(filteredStations[i].id)
    return ids
  }

  function requestMapColorRefresh() {
    mapOverlayRefreshTimer.restart()
  }

  function markerColorForStation(station) {
    if (selectedStationId === station.id)
      return "#FF7A18"

    var metrics = App.mapStationMetrics || {}
    var entry = metrics[String(station.id)]
    if (!entry)
      return "#00D8FF"

    if (entry.status === "loading")
      return "#78B9C8"
    if (entry.status === "missing")
      return "#6E7D86"
    if (entry.status === "error")
      return "#C35D71"

    var range = App.mapMetricRange || {}
    var minV = Number(range.min)
    var maxV = Number(range.max)
    var value = Number(entry.value)

    if (!isFinite(value))
      return "#00D8FF"
    if (!isFinite(minV) || !isFinite(maxV) || maxV <= minV)
      return "#F7C84B"

    var t = (value - minV) / (maxV - minV)
    if (t < 0.25)
      return "#38D97A"
    if (t < 0.5)
      return "#B4E34C"
    if (t < 0.75)
      return "#FFB347"
    return "#FF5D5D"
  }

  function stationFilterStatusText() {
    if (addressLookupMessage.length > 0
        && addressLookupMessage !== "Address resolved successfully.")
      return addressLookupMessage
    return radiusSearchMessage
  }

  function stationFilterStatusColor() {
    var message = stationFilterStatusText()
    if (!message.length)
      return "#CCF3FB"
    if (message.indexOf("Address lookup failed") === 0
        || message.indexOf("No matching address") === 0)
      return "#FFB0B0"
    if (message.indexOf("Resolving address") === 0)
      return "#CCF3FB"
    return radiusFilterEnabled ? "#8AF5B2" : "#CCF3FB"
  }

  function mapLegendParamCode() {
    var range = App.mapMetricRange || {}
    if (range.paramLabel)
      return String(range.paramLabel)
    if (range.paramCode)
      return String(range.paramCode)
    if (App.currentParamCode && App.currentParamCode.length > 0)
      return App.currentParamCode
    return ""
  }

  function hasMapLegend() {
    var range = App.mapMetricRange || {}
    return mapLegendParamCode().length > 0
        && isFinite(Number(range.min))
        && isFinite(Number(range.max))
        && Number(range.count) > 0
  }

  function formatLegendValue(value) {
    var numericValue = Number(value)
    if (!isFinite(numericValue))
      return "-"
    if (Math.abs(numericValue) >= 100)
      return numericValue.toFixed(0)
    return numericValue.toFixed(1)
  }

  function stationMatchesFilter(station, rawQuery) {
    var query = rawQuery ? rawQuery.trim().toLowerCase() : ""
    if (!query.length)
      return true

    var haystack = [
      station.name || "",
      station.city || "",
      station.address || ""
    ].join(" ").toLowerCase()
    return haystack.indexOf(query) >= 0
  }

  function updateFilteredStations() {
    var stations = App.stations || []
    var nextStations = []
    var hasSelectedStation = false

    for (var i = 0; i < stations.length; i++) {
      var station = stations[i]
      if (!stationMatchesFilter(station, cityFilter ? cityFilter.text : ""))
        continue

      var distanceKm = stationDistanceKm(station)
      if (radiusFilterEnabled) {
        if (distanceKm === null || distanceKm > radiusKm.value)
          continue
      }

      var stationEntry = {}
      for (var key in station)
        stationEntry[key] = station[key]
      if (distanceKm !== null)
        stationEntry.distanceKm = distanceKm

      nextStations.push(stationEntry)
      if (station.id === selectedStationId)
          hasSelectedStation = true
    }

    if (radiusFilterEnabled) {
      nextStations.sort(function(a, b) {
        return (a.distanceKm || 0) - (b.distanceKm || 0)
      })
      radiusSearchMessage = "Showing " + nextStations.length + " station(s) within "
        + radiusKm.value + " km of " + radiusSearchAddress + "."
    } else if (radiusSearchMessage.indexOf("Showing ") === 0) {
      radiusSearchMessage = ""
    }

    filteredStations = nextStations
    if (!hasSelectedStation && selectedStationId !== -1)
      selectedStationId = -1
    updateMapViewState()
    requestMapColorRefresh()
  }

  Timer {
    id: mapOverlayRefreshTimer
    interval: 240
    repeat: false
    onTriggered: App.refreshMapMeasurements(currentFilteredStationIds(), App.currentParamCode)
  }

  Plugin {
    id: mapPlugin
    name: "osm"
  }

  GeocodeModel {
    id: geocodeModel
    plugin: mapPlugin
    autoUpdate: false

    onLocationsChanged: {
      if (count > 0) {
        radiusSearchCenter = get(0).coordinate
        radiusSearchAddress = addressSearch.text.trim()
        radiusFilterEnabled = true
        addressLookupMessage = "Address resolved successfully."
        updateFilteredStations()
      } else {
        radiusSearchCenter = QtPositioning.coordinate()
        radiusSearchAddress = ""
        radiusFilterEnabled = false
        addressLookupMessage = "No matching address was found."
        updateFilteredStations()
      }
    }

    onStatusChanged: {
      if (status === GeocodeModel.Loading) {
        addressLookupMessage = "Resolving address..."
      } else if (status === GeocodeModel.Error) {
        radiusSearchCenter = QtPositioning.coordinate()
        radiusSearchAddress = ""
        radiusFilterEnabled = false
        addressLookupMessage = "Address lookup failed: " + errorString
        updateFilteredStations()
      }
    }
  }

  Rectangle {
    anchors.fill: parent
    gradient: Gradient {
      GradientStop { position: 0.0; color: "#0B0F14" }
      GradientStop { position: 1.0; color: "#070A0E" }
    }

    Rectangle {
      anchors.fill: parent
      anchors.margins: 20
      radius: 26
      color: "#0DFFFFFF"
      border.color: "#1CFFFFFF"
      border.width: 1

      ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        RowLayout {
          Layout.fillWidth: true
          spacing: 16

          Item {
            Layout.fillWidth: true
            implicitHeight: headerColumn.implicitHeight

            Column {
              id: headerColumn
              anchors.left: parent.left
              anchors.right: parent.right
              spacing: 4

              Text {
                text: "Air Quality Monitor"
                color: "white"
                font.pixelSize: 22
                font.bold: true
              }

              Text {
                width: parent.width
                text: App.banner
                color: "#B3FFFFFF"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
              }
            }
          }

          Rectangle {
            Layout.alignment: Qt.AlignTop
            Layout.preferredWidth: 120
            Layout.preferredHeight: 34
            radius: 12
            color: App.offline ? "#22FF7A18" : "#2200FF99"
            border.color: "#22FFFFFF"
            border.width: 1

            Text {
              anchors.centerIn: parent
              text: App.offline ? "OFFLINE" : "ONLINE"
              color: "white"
              font.pixelSize: 12
              font.bold: true
            }
          }
        }

        RowLayout {
          id: contentRow
          Layout.fillWidth: true
          Layout.fillHeight: true
          spacing: 16

          ColumnLayout {
            Layout.fillHeight: true
            Layout.minimumWidth: 340
            Layout.preferredWidth: Math.max(350, contentRow.width * 0.32)
            Layout.maximumWidth: Math.max(420, contentRow.width * 0.38)
            spacing: 16

            GlassCard {
              Layout.fillWidth: true
              Layout.fillHeight: true
              Layout.minimumHeight: 320
              Layout.preferredHeight: Math.max(340, contentRow.height * 0.50)

              ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

                RowLayout {
                  Layout.fillWidth: true
                  spacing: 10

                  Text {
                    text: "Stations"
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                  }

                  Item { Layout.fillWidth: true }

                  GlassButton {
                    text: "Download"
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 38
                    onClicked: {
                      selectedStationId = -1
                      App.refreshStations()
                    }
                  }
                }

                TextField {
                  id: cityFilter
                  Layout.fillWidth: true
                  Layout.preferredHeight: 38
                  placeholderText: "Filter by city"
                  onTextChanged: updateFilteredStations()
                }

                TextField {
                  id: addressSearch
                  Layout.fillWidth: true
                  Layout.preferredHeight: 38
                  placeholderText: "Address for radius search"
                  onAccepted: {
                    if (text.trim().length > 0) {
                      geocodeModel.query = text.trim()
                      geocodeModel.update()
                    }
                  }
                }

                RowLayout {
                  Layout.fillWidth: true
                  spacing: 8

                  Label {
                    text: "Radius (km)"
                    color: "#C7F6FF"
                    font.pixelSize: 12
                  }

                  SpinBox {
                    id: radiusKm
                    from: 1
                    to: 250
                    value: 25
                    editable: true
                    Layout.preferredWidth: 88
                    Layout.preferredHeight: 38
                    leftPadding: 20
                    rightPadding: 20

                    contentItem: TextInput {
                      text: radiusKm.textFromValue(radiusKm.value, radiusKm.locale)
                      font: radiusKm.font
                      color: "white"
                      selectionColor: "#33A8F0FF"
                      selectedTextColor: "white"
                      horizontalAlignment: Qt.AlignHCenter
                      verticalAlignment: Qt.AlignVCenter
                      readOnly: !radiusKm.editable
                      validator: radiusKm.validator
                      inputMethodHints: Qt.ImhFormattedNumbersOnly
                    }

                    background: Rectangle {
                      radius: 10
                      color: "#1AFFFFFF"
                      border.color: "#22FFFFFF"
                      border.width: 1
                    }

                    up.indicator: Rectangle {
                      x: parent.width - width
                      y: 0
                      implicitWidth: 20
                      implicitHeight: parent.height
                      height: parent.height
                      color: radiusKm.up.pressed ? "#24FFFFFF" : "transparent"

                      Text {
                        anchors.centerIn: parent
                        text: "+"
                        color: "white"
                        font.pixelSize: 20
                      }
                    }

                    down.indicator: Rectangle {
                      x: 0
                      y: 0
                      implicitWidth: 20
                      implicitHeight: parent.height
                      height: parent.height
                      color: radiusKm.down.pressed ? "#24FFFFFF" : "transparent"

                      Text {
                        anchors.centerIn: parent
                        text: "-"
                        color: "white"
                        font.pixelSize: 20
                      }
                    }

                    onValueModified: {
                      if (radiusFilterEnabled)
                        updateFilteredStations()
                    }
                  }

                  GlassButton {
                    text: "Locate"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    enabled: addressSearch.text.trim().length > 0
                    onClicked: {
                      geocodeModel.query = addressSearch.text.trim()
                      geocodeModel.update()
                    }
                  }

                  GlassButton {
                    text: "Clear"
                    Layout.preferredWidth: 82
                    Layout.preferredHeight: 38
                    enabled: radiusFilterEnabled
                    onClicked: clearRadiusSearch()
                  }
                }

                Text {
                  Layout.fillWidth: true
                  visible: stationFilterStatusText().length > 0
                  text: stationFilterStatusText()
                  color: stationFilterStatusColor()
                  font.pixelSize: 10
                  wrapMode: Text.WordWrap
                  maximumLineCount: 2
                  elide: Text.ElideRight
                }

                ListView {
                  id: stationList
                  Layout.fillWidth: true
                  Layout.fillHeight: true
                  Layout.minimumHeight: 84
                  clip: true
                  model: filteredStations
                  spacing: 8
                  ScrollBar.vertical: ScrollBar {}

                  delegate: Rectangle {
                    property var st: modelData

                    width: stationList.width
                    height: 42
                    radius: 12
                    color: selectedStationId === st.id ? "#22FF7A18" : "#10FFFFFF"
                    border.color: selectedStationId === st.id ? "#66FF7A18" : "#22FFFFFF"
                    border.width: 1

                    Text {
                      anchors.left: parent.left
                      anchors.leftMargin: 12
                      anchors.right: parent.right
                      anchors.rightMargin: 12
                      anchors.verticalCenter: parent.verticalCenter
                      text: st.name + " - " + st.city
                            + (st.distanceKm !== undefined ? (" (" + st.distanceKm.toFixed(1) + " km)") : "")
                      color: "white"
                      font.pixelSize: 12
                      elide: Text.ElideRight
                    }

                    MouseArea {
                      anchors.fill: parent
                      onClicked: {
                        selectedStationId = st.id
                        App.loadSensors(st.id)
                      }
                    }
                  }
                }
              }
            }

            GlassCard {
              Layout.fillWidth: true
              Layout.fillHeight: true
              Layout.minimumHeight: 300

              ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

                Text {
                  text: "Sensors"
                  color: "white"
                  font.pixelSize: 16
                  font.bold: true
                }

                ListView {
                  id: sensorList
                  Layout.fillWidth: true
                  Layout.fillHeight: true
                  Layout.minimumHeight: 100
                  clip: true
                  model: App.sensors
                  spacing: 8
                  ScrollBar.vertical: ScrollBar {}

                  delegate: Rectangle {
                    property var s: modelData

                    width: sensorList.width
                    height: 42
                    radius: 12
                    color: App.currentSensorId === s.id ? "#22FF7A18" : "#10FFFFFF"
                    border.color: App.currentSensorId === s.id ? "#66FF7A18" : "#22FFFFFF"
                    border.width: 1

                    Text {
                      anchors.left: parent.left
                      anchors.leftMargin: 12
                      anchors.right: parent.right
                      anchors.rightMargin: 12
                      anchors.verticalCenter: parent.verticalCenter
                      text: (s.displayName && s.displayName.length > 0 ? s.displayName : s.paramName) + " (" + s.paramCode + ")"
                      color: "white"
                      font.pixelSize: 12
                      elide: Text.ElideRight
                    }

                    MouseArea {
                      anchors.fill: parent
                      onClicked: App.loadOnline(s.id)
                    }
                  }
                }

                GlassButton {
                  text: "Save to Local DB"
                  Layout.fillWidth: true
                  Layout.preferredHeight: 42
                  onClicked: App.saveCurrentToDb()
                }

                RowLayout {
                  Layout.fillWidth: true
                  spacing: 10

                  Label {
                    text: "History range"
                    color: "#C7F6FF"
                    font.pixelSize: 12
                  }

                  SpinBox {
                    id: historyDays
                    from: 1
                    to: 30
                    value: 7
                    editable: true
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 42
                    leftPadding: 20
                    rightPadding: 20

                    contentItem: TextInput {
                      text: historyDays.textFromValue(historyDays.value, historyDays.locale)
                      font: historyDays.font
                      color: "white"
                      selectionColor: "#33A8F0FF"
                      selectedTextColor: "white"
                      horizontalAlignment: Qt.AlignHCenter
                      verticalAlignment: Qt.AlignVCenter
                      readOnly: !historyDays.editable
                      validator: historyDays.validator
                      inputMethodHints: Qt.ImhFormattedNumbersOnly
                    }

                    background: Rectangle {
                      radius: 10
                      color: "#1AFFFFFF"
                      border.color: "#22FFFFFF"
                      border.width: 1
                    }

                    up.indicator: Rectangle {
                      x: parent.width - width
                      y: 0
                      implicitWidth: 20
                      implicitHeight: parent.height
                      height: parent.height
                      color: historyDays.up.pressed ? "#24FFFFFF" : "transparent"
                      border.color: "transparent"

                      Text {
                        anchors.centerIn: parent
                        text: "+"
                        color: "white"
                        font.pixelSize: 22
                      }
                    }

                    down.indicator: Rectangle {
                      x: 0
                      y: 0
                      implicitWidth: 20
                      implicitHeight: parent.height
                      height: parent.height
                      color: historyDays.down.pressed ? "#24FFFFFF" : "transparent"
                      border.color: "transparent"

                      Text {
                        anchors.centerIn: parent
                        text: "-"
                        color: "white"
                        font.pixelSize: 22
                      }
                    }
                  }

                  GlassButton {
                    text: "Load Local History"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    enabled: App.historyAvailable
                    opacity: enabled ? 1.0 : 0.45
                    onClicked: App.loadCurrentHistory(historyDays.value)
                  }
                }

                Text {
                  Layout.fillWidth: true
                  text: App.currentSensorId > 0
                        ? ("Selected sensor: " + App.currentSensorId + " | click a sensor to refresh the chart | range: " + historyDays.value + " days")
                        : "Select a sensor to load online measurements and update the chart."
                  color: "#99FFFFFF"
                  font.pixelSize: 11
                  wrapMode: Text.WordWrap
                }

                Text {
                  Layout.fillWidth: true
                  visible: App.historyAvailable
                  text: "Local history is available for the selected sensor."
                  color: "#8AF5B2"
                  font.pixelSize: 11
                  wrapMode: Text.WordWrap
                }
              }
            }
          }

          ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16

            GlassCard {
              Layout.fillWidth: true
              Layout.fillHeight: true
              Layout.minimumHeight: 420

              ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                RowLayout {
                  Layout.fillWidth: true
                  spacing: 16

                  Text {
                    text: "Data Views"
                    color: "white"
                    font.pixelSize: 16
                    font.bold: true
                  }

                  Item { Layout.fillWidth: true }

                  TabBar {
                    id: viewTabs
                    Layout.preferredWidth: 220
                    implicitHeight: 38

                    background: Rectangle {
                      radius: 12
                      color: "#12FFFFFF"
                      border.color: "#22FFFFFF"
                      border.width: 1
                    }

                    TabButton { text: "Chart" }
                    TabButton { text: "Map" }
                  }
                }

                StackLayout {
                  Layout.fillWidth: true
                  Layout.fillHeight: true
                  currentIndex: viewTabs.currentIndex

                  Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ChartView {
                      id: chart
                      anchors.fill: parent
                      antialiasing: true
                      legend.visible: false
                      backgroundColor: "transparent"
                      plotAreaColor: "#12000000"

                      ValueAxis {
                        id: axY
                        min: 0
                        max: 200
                        labelsColor: "#EAF7FF"
                        color: "#A8F0FF"
                        gridLineColor: "#55D9F7FF"
                      }
                      DateTimeAxis {
                        id: axX
                        format: "dd.MM HH:mm"
                        tickCount: 6
                        labelsColor: "#EAF7FF"
                        color: "#A8F0FF"
                        gridLineColor: "#55D9F7FF"
                      }

                      LineSeries {
                        id: series
                        axisX: axX
                        axisY: axY
                        color: "#00D8FF"
                        width: 3
                        pointsVisible: true
                        pointLabelsVisible: false
                      }

                      function refresh() {
                        series.clear()
                        var pts = App.chartPoints
                        if (!pts || pts.length === 0)
                          return

                        var minV = 1e9
                        var maxV = -1e9
                        var minT = Number.MAX_SAFE_INTEGER
                        var maxT = 0

                        for (var i = 0; i < pts.length; i++) {
                          var t = pts[i]["t"]
                          var v = pts[i]["v"]
                          series.append(t, v)
                          if (v < minV) minV = v
                          if (v > maxV) maxV = v
                          if (t < minT) minT = t
                          if (t > maxT) maxT = t
                        }

                        var yPad = Math.max(2, (maxV - minV) * 0.12)
                        axY.min = Math.max(0, minV - yPad)
                        axY.max = maxV + yPad

                        var span = maxT - minT
                        var xPad = span > 0 ? Math.max(60 * 60 * 1000, span * 0.05) : 60 * 60 * 1000
                        axX.min = new Date(minT - xPad)
                        axX.max = new Date(maxT + xPad)
                      }

                      Connections {
                        target: App
                        function onChartPointsChanged() { chart.refresh() }
                      }

                      Text {
                        anchors.centerIn: parent
                        visible: App.chartPoints.length === 0
                        text: "No data to display"
                        color: "#CCF3FB"
                        font.pixelSize: 16
                      }
                    }
                  }

                  Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Map {
                      id: stationMap
                      anchors.fill: parent
                      plugin: mapPlugin
                      center: currentMapCenter
                      zoomLevel: currentMapZoomLevel
                      copyrightsVisible: false
                      activeMapType: supportedMapTypes.length > 0 ? supportedMapTypes[0] : null

                      Repeater {
                        model: filteredStations

                        delegate: MapQuickItem {
                          property var station: modelData
                          coordinate: stationCoordinate(station)
                          anchorPoint.x: 9
                          anchorPoint.y: 9
                          visible: hasValidCoordinate(coordinate)

                          sourceItem: Rectangle {
                            width: 18
                            height: 18
                            radius: 9
                            color: markerColorForStation(station)
                            border.color: "white"
                            border.width: 2

                            MouseArea {
                              anchors.fill: parent
                              onClicked: {
                                selectedStationId = station.id
                                App.loadSensors(station.id)
                              }
                            }
                          }
                        }
                      }

                      MapQuickItem {
                        visible: radiusFilterEnabled && hasValidCoordinate(radiusSearchCenter)
                        coordinate: radiusSearchCenter
                        anchorPoint.x: 12
                        anchorPoint.y: 12

                        sourceItem: Rectangle {
                          width: 24
                          height: 24
                          radius: 12
                          color: "#5520E3FF"
                          border.color: "#9AEFFFFF"
                          border.width: 2
                        }
                      }
                    }

                    Rectangle {
                      anchors.left: parent.left
                      anchors.leftMargin: 12
                      anchors.top: parent.top
                      anchors.topMargin: 12
                      radius: 12
                      color: "#7A0B1016"
                      border.color: "#22FFFFFF"
                      border.width: 1
                      visible: radiusSearchMessage.length > 0
                               || addressLookupMessage.length > 0
                               || App.mapOverlayStatus.length > 0
                      width: Math.min(parent.width * 0.72, 420)
                      height: mapOverlayColumn.implicitHeight + 20

                      Column {
                        id: mapOverlayColumn
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 6

                        Text {
                          width: parent.width
                          visible: stationFilterStatusText().length > 0
                          text: stationFilterStatusText()
                          color: stationFilterStatusColor()
                          font.pixelSize: 11
                          wrapMode: Text.WordWrap
                        }

                        Text {
                          width: parent.width
                          visible: App.mapOverlayStatus.length > 0
                          text: App.mapOverlayStatus
                          color: "#ECFBFF"
                          font.pixelSize: 11
                          wrapMode: Text.WordWrap
                        }
                      }
                    }

                    Rectangle {
                      anchors.right: parent.right
                      anchors.rightMargin: 12
                      anchors.bottom: parent.bottom
                      anchors.bottomMargin: 12
                      radius: 12
                      color: "#7A0B1016"
                      border.color: "#22FFFFFF"
                      border.width: 1
                      visible: hasMapLegend()
                      width: Math.min(parent.width * 0.38, 260)
                      height: legendColumn.implicitHeight + 20

                      Column {
                        id: legendColumn
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 8

                        Text {
                          width: parent.width
                          text: "Color scale: " + mapLegendParamCode()
                          color: "white"
                          font.pixelSize: 12
                          font.bold: true
                          wrapMode: Text.WordWrap
                        }

                        Rectangle {
                          width: parent.width
                          height: 12
                          radius: 6
                          gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: "#38D97A" }
                            GradientStop { position: 0.35; color: "#B4E34C" }
                            GradientStop { position: 0.7; color: "#FFB347" }
                            GradientStop { position: 1.0; color: "#FF5D5D" }
                          }
                        }

                        RowLayout {
                          width: parent.width
                          spacing: 8

                          Column {
                            spacing: 2

                            Text {
                              text: "low"
                              color: "#CCFFFFFF"
                              font.pixelSize: 10
                            }

                            Text {
                              text: formatLegendValue((App.mapMetricRange || {}).min)
                              color: "#DDF9FF"
                              font.pixelSize: 10
                              font.bold: true
                            }
                          }

                          Item { Layout.fillWidth: true }

                          Column {
                            spacing: 2

                            Text {
                              text: "high"
                              color: "#CCFFFFFF"
                              font.pixelSize: 10
                              horizontalAlignment: Text.AlignRight
                            }

                            Text {
                              text: formatLegendValue((App.mapMetricRange || {}).max)
                              color: "#DDF9FF"
                              font.pixelSize: 10
                              font.bold: true
                              horizontalAlignment: Text.AlignRight
                            }
                          }
                        }

                        Row {
                          spacing: 6

                          Rectangle {
                            width: 10
                            height: 10
                            radius: 5
                            color: "#FF7A18"
                            border.color: "white"
                            border.width: 1
                          }

                          Text {
                            text: "selected station"
                            color: "#CCFFFFFF"
                            font.pixelSize: 10
                          }
                        }
                      }
                    }
                  }
                }
              }
            }

            GlassCard {
              Layout.fillWidth: true
              Layout.preferredHeight: 170
              Layout.minimumHeight: 170

              ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 10

                Text {
                  text: "Analysis"
                  color: "white"
                  font.pixelSize: 16
                  font.bold: true
                }

                Flow {
                  Layout.fillWidth: true
                  spacing: 18

                  Text { text: App.stats.ok ? ("min: " + App.stats.min.toFixed(2)) : "min: -"; color: "#CCFFFFFF"; font.pixelSize: 12 }
                  Text { text: App.stats.ok ? ("max: " + App.stats.max.toFixed(2)) : "max: -"; color: "#CCFFFFFF"; font.pixelSize: 12 }
                  Text { text: App.stats.ok ? ("avg: " + App.stats.avg.toFixed(2)) : "avg: -"; color: "#CCFFFFFF"; font.pixelSize: 12 }
                  Text { text: App.stats.ok ? ("trend: " + App.stats.trend) : "trend: -"; color: "#CCFFFFFF"; font.pixelSize: 12 }
                }

                Text {
                  Layout.fillWidth: true
                  text: App.stats.ok ? ("minAt: " + App.stats.minAt) : "minAt: -"
                  color: "#99FFFFFF"
                  font.pixelSize: 11
                  wrapMode: Text.WordWrap
                }

                Text {
                  Layout.fillWidth: true
                  text: App.stats.ok ? ("maxAt: " + App.stats.maxAt) : "maxAt: -"
                  color: "#99FFFFFF"
                  font.pixelSize: 11
                  wrapMode: Text.WordWrap
                }
              }
            }
          }
        }
      }
    }
  }

  Connections {
    target: App
    function onStationsChanged() { updateFilteredStations() }
    function onCurrentParamCodeChanged() { requestMapColorRefresh() }
  }

  onSelectedStationIdChanged: updateMapViewState()

  Component.onCompleted: updateFilteredStations()
}
