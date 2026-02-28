import QtQuick
import MediaConsole 1.0

Item {
    id: alphabetSidebar
    width: 30
    signal letterSelected(string letter)

    property string currentLetter: ""
    property var letters: ["#","A","B","C","D","E","F","G","H","I","J","K","L",
                           "M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z"]

    Column {
        anchors.fill: parent

        Repeater {
            model: alphabetSidebar.letters

            Text {
                text: modelData
                color: alphabetSidebar.currentLetter === modelData ? Theme.accent : Theme.textSecondary
                font.pixelSize: 11
                font.bold: alphabetSidebar.currentLetter === modelData
                width: alphabetSidebar.width
                horizontalAlignment: Text.AlignHCenter
                height: alphabetSidebar.height / alphabetSidebar.letters.length
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        preventStealing: true

        onPressed: function(mouse) { updateLetter(mouse.y) }
        onPositionChanged: function(mouse) { updateLetter(mouse.y) }
        onReleased: alphabetSidebar.currentLetter = ""

        function updateLetter(y) {
            var idx = Math.floor(y / (height / alphabetSidebar.letters.length))
            idx = Math.max(0, Math.min(idx, alphabetSidebar.letters.length - 1))
            alphabetSidebar.currentLetter = alphabetSidebar.letters[idx]
            alphabetSidebar.letterSelected(alphabetSidebar.letters[idx])
        }
    }

    // Floating preview bubble — appears to the left of the sidebar
    Rectangle {
        visible: alphabetSidebar.currentLetter !== ""
        width: 60
        height: 60
        radius: 30
        color: Theme.glassBg
        border.color: Theme.glassBorder
        border.width: 1
        x: -80
        y: {
            var idx = alphabetSidebar.letters.indexOf(alphabetSidebar.currentLetter)
            if (idx < 0) return 0
            return (idx / alphabetSidebar.letters.length) * alphabetSidebar.height - 30
        }

        Text {
            anchors.centerIn: parent
            text: alphabetSidebar.currentLetter
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeLarge
            font.bold: true
        }
    }
}
