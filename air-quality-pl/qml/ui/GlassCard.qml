import QtQuick 2.15

Item {
  id: root
  property real radius: 18
  property real opacityGlass: 0.10
  property color borderColor: "#2AFFFFFF"
  property color glassTint: "#FFFFFF"
  property real borderWidth: 1
  clip: true

  implicitWidth: 220
  implicitHeight: 140

  // Karta daje półprzezroczyste tło i obramowanie dla głównych sekcji interfejsu.
  Rectangle {
    anchors.fill: parent
    radius: root.radius
    color: Qt.rgba(1, 1, 1, root.opacityGlass)
    border.color: root.borderColor
    border.width: root.borderWidth

    Rectangle {
      anchors.fill: parent
      radius: root.radius
      gradient: Gradient {
        GradientStop { position: 0.0; color: "#1FFFFFFF" }
        GradientStop { position: 1.0; color: "#05FFFFFF" }
      }
    }
  }
}
