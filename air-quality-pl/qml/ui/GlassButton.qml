import QtQuick 2.15
import QtQuick.Controls 2.15

Button {
  id: btn
  property color accent: "#FF7A18"

  background: Rectangle {
    radius: 14
    color: !btn.enabled ? "#667A7A7A" : (btn.down ? Qt.darker(btn.accent, 1.2) : btn.accent)
    border.color: "#22FFFFFF"
    border.width: 1
    opacity: btn.enabled ? 1.0 : 0.7
  }

  contentItem: Text {
    text: btn.text
    color: btn.enabled ? "white" : "#CCFFFFFF"
    font.pixelSize: 14
    font.bold: true
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
  }

  padding: 14
}
