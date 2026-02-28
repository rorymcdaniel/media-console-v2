import QtQuick
import QtQuick.Controls
import MediaConsole 1.0

Item {
    id: root

    // === Data model: 6 input sources ===
    ListModel {
        id: sourceModel
        ListElement { sourceValue: 1; labelText: "Streaming"; iconText: "\u266B" }
        ListElement { sourceValue: 2; labelText: "Phono"; iconText: "\u25CE" }
        ListElement { sourceValue: 3; labelText: "CD"; iconText: "\u25CF" }
        ListElement { sourceValue: 4; labelText: "Computer"; iconText: "\u2338" }
        ListElement { sourceValue: 5; labelText: "Bluetooth"; iconText: "\u2261" }
        ListElement { sourceValue: 6; labelText: "Library"; iconText: "\u2630" }
    }

    // Map MediaSource enum to model index
    function sourceToIndex(source) {
        switch (source) {
            case MediaSource.Streaming: return 0
            case MediaSource.Phono:     return 1
            case MediaSource.CD:        return 2
            case MediaSource.Computer:  return 3
            case MediaSource.Bluetooth: return 4
            case MediaSource.Library:   return 5
            default: return -1
        }
    }

    // Map model index to MediaSource enum
    function indexToSource(index) {
        switch (index) {
            case 0: return MediaSource.Streaming
            case 1: return MediaSource.Phono
            case 2: return MediaSource.CD
            case 3: return MediaSource.Computer
            case 4: return MediaSource.Bluetooth
            case 5: return MediaSource.Library
            default: return MediaSource.None
        }
    }

    // === Auto-select state ===
    property int focusedIndex: sourceToIndex(ReceiverState.currentInput)
    property real autoSelectProgress: 0.0

    Timer {
        id: autoSelectTimer
        interval: 40  // ~25fps update for smooth ring animation
        repeat: true
        onTriggered: {
            autoSelectProgress += (40 / 4000)  // 4 seconds total
            if (autoSelectProgress >= 1.0) {
                autoSelectTimer.stop()
                autoSelectProgress = 1.0
                ReceiverController.selectInput(indexToSource(focusedIndex))
            }
        }
    }

    function focusInput(index) {
        if (index === sourceToIndex(ReceiverState.currentInput)) {
            // Already the active input, no auto-select needed
            autoSelectTimer.stop()
            autoSelectProgress = 0.0
            return
        }
        focusedIndex = index
        autoSelectProgress = 0.0
        autoSelectTimer.restart()
    }

    function selectImmediately(index) {
        autoSelectTimer.stop()
        autoSelectProgress = 0.0
        focusedIndex = index
        ReceiverController.selectInput(indexToSource(index))
    }

    // Sync with external input changes
    Connections {
        target: ReceiverState
        function onCurrentInputChanged() {
            var idx = sourceToIndex(ReceiverState.currentInput)
            if (idx >= 0) {
                root.focusedIndex = idx
                root.autoSelectProgress = 0.0
                autoSelectTimer.stop()
                carousel.positionViewAtIndex(idx, ListView.Center)
            }
        }
    }

    // === Vertical Carousel ===
    ListView {
        id: carousel
        anchors.fill: parent
        anchors.topMargin: Theme.spacingLarge
        anchors.bottomMargin: Theme.spacingLarge
        model: sourceModel
        orientation: ListView.Vertical
        snapMode: ListView.SnapToItem
        preferredHighlightBegin: (height - itemHeight) / 2
        preferredHighlightEnd: (height + itemHeight) / 2
        highlightRangeMode: ListView.StrictlyEnforceRange
        clip: true
        currentIndex: root.focusedIndex >= 0 ? root.focusedIndex : 0
        interactive: true

        property int itemHeight: 110

        onCurrentIndexChanged: {
            root.focusInput(currentIndex)
        }

        delegate: Item {
            id: delegateItem
            width: carousel.width
            height: carousel.itemHeight

            required property int index
            required property string labelText
            required property string iconText
            required property int sourceValue

            // Distance from center for scaling/opacity
            property real distFromCenter: {
                var center = carousel.contentY + carousel.height / 2
                var itemCenter = y + height / 2
                return Math.abs(itemCenter - center) / carousel.itemHeight
            }

            property bool isCurrent: ReceiverState.currentInput === root.indexToSource(index)
            property bool isFocused: root.focusedIndex === index

            // 3D perspective: scale and opacity based on distance from center
            property real itemScale: {
                if (distFromCenter < 0.5) return 1.0
                if (distFromCenter < 1.5) return 0.65
                return 0.4
            }

            property real itemOpacity: {
                if (distFromCenter < 0.5) return 1.0
                if (distFromCenter < 1.5) return 0.5
                return 0.25
            }

            // 3D rotation angle based on distance
            property real displaceAngle: {
                var center = carousel.contentY + carousel.height / 2
                var itemCenter = y + height / 2
                var dist = (itemCenter - center) / carousel.itemHeight
                return dist * 25  // 25 degrees per item offset
            }

            transform: [
                Rotation {
                    origin.x: delegateItem.width / 2
                    origin.y: delegateItem.height / 2
                    axis { x: 1; y: 0; z: 0 }
                    angle: delegateItem.displaceAngle
                }
            ]

            scale: itemScale
            opacity: itemOpacity

            Behavior on scale {
                NumberAnimation { duration: Theme.animMedium; easing.type: Easing.OutCubic }
            }
            Behavior on opacity {
                NumberAnimation { duration: Theme.animMedium; easing.type: Easing.OutCubic }
            }

            Column {
                anchors.centerIn: parent
                spacing: Theme.spacingSmall

                // Icon circle with optional progress ring
                Item {
                    width: 72
                    height: 72
                    anchors.horizontalCenter: parent.horizontalCenter

                    // Progress ring (auto-select countdown)
                    Canvas {
                        id: progressRing
                        anchors.fill: parent
                        anchors.margins: -4
                        width: 80
                        height: 80
                        visible: delegateItem.isFocused && root.autoSelectProgress > 0

                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            ctx.strokeStyle = Theme.accentLight.toString()
                            ctx.lineWidth = 3
                            ctx.lineCap = "round"
                            ctx.beginPath()
                            var startAngle = -Math.PI / 2
                            var endAngle = startAngle + (2 * Math.PI * root.autoSelectProgress)
                            ctx.arc(width / 2, height / 2, width / 2 - 2, startAngle, endAngle)
                            ctx.stroke()
                        }

                        // Repaint when progress changes
                        Connections {
                            target: root
                            function onAutoSelectProgressChanged() {
                                if (delegateItem.isFocused) {
                                    progressRing.requestPaint()
                                }
                            }
                        }
                    }

                    // Icon background circle
                    Rectangle {
                        id: iconCircle
                        anchors.centerIn: parent
                        width: 64
                        height: 64
                        radius: 32
                        color: delegateItem.isCurrent ? Theme.accent : Theme.glassBg
                        border.color: delegateItem.isCurrent ? Theme.accentLight : Theme.glassBorder
                        border.width: 2

                        Behavior on color {
                            ColorAnimation { duration: Theme.animMedium }
                        }
                        Behavior on border.color {
                            ColorAnimation { duration: Theme.animMedium }
                        }

                        Text {
                            anchors.centerIn: parent
                            text: delegateItem.iconText
                            font.pixelSize: 28
                            color: Theme.textPrimary
                        }
                    }
                }

                // Source label
                Text {
                    text: delegateItem.labelText
                    color: delegateItem.isCurrent ? Theme.textPrimary : Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSmall
                    font.bold: delegateItem.isCurrent
                    anchors.horizontalCenter: parent.horizontalCenter

                    Behavior on color {
                        ColorAnimation { duration: Theme.animMedium }
                    }
                }
            }

            // Tap to select immediately
            MouseArea {
                anchors.fill: parent
                onClicked: root.selectImmediately(delegateItem.index)
            }
        }

        // Initialize position
        Component.onCompleted: {
            var idx = root.sourceToIndex(ReceiverState.currentInput)
            if (idx >= 0) {
                positionViewAtIndex(idx, ListView.Center)
            }
        }
    }
}
