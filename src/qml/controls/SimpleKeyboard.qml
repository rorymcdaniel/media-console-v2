import QtQuick
import MediaConsole 1.0

Item {
    id: keyboard
    signal keyPressed(string key)

    property var rows: [
        ["1","2","3","4","5","6","7","8","9","0"],
        ["Q","W","E","R","T","Y","U","I","O","P"],
        ["A","S","D","F","G","H","J","K","L"],
        ["Z","X","C","V","B","N","M"]
    ]

    Column {
        anchors.fill: parent
        spacing: 4

        Repeater {
            model: keyboard.rows

            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 4

                Repeater {
                    model: modelData

                    Rectangle {
                        width: 56
                        height: Theme.touchTargetSmall
                        radius: Theme.radiusSmall
                        color: letterKeyMa.pressed ? Theme.accent : Theme.glassBg
                        border.color: Theme.glassBorder
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: modelData
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeBody
                        }

                        MouseArea {
                            id: letterKeyMa
                            anchors.fill: parent
                            onClicked: keyboard.keyPressed(modelData.toLowerCase())
                        }
                    }
                }
            }
        }

        // Bottom row: Space, Backspace
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 4

            // Space bar
            Rectangle {
                width: 300
                height: Theme.touchTargetSmall
                radius: Theme.radiusSmall
                color: spaceMa.pressed ? Theme.accent : Theme.glassBg
                border.color: Theme.glassBorder
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "SPACE"
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSmall
                }

                MouseArea {
                    id: spaceMa
                    anchors.fill: parent
                    onClicked: keyboard.keyPressed(" ")
                }
            }

            // Backspace
            Rectangle {
                width: 80
                height: Theme.touchTargetSmall
                radius: Theme.radiusSmall
                color: bkspMa.pressed ? Theme.accent : Theme.glassBg
                border.color: Theme.glassBorder
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: "\u232B"
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMedium
                }

                MouseArea {
                    id: bkspMa
                    anchors.fill: parent
                    onClicked: keyboard.keyPressed("backspace")
                }
            }
        }
    }
}
