import QtQuick
import MediaConsole 1.0

Item {
    id: root
    anchors.fill: parent

    // === Data model: 6 input sources ===
    ListModel {
        id: sourceModel
        ListElement { sourceValue: 1; labelText: "Streaming"; iconText: "\u266B" }
        ListElement { sourceValue: 2; labelText: "Phono";     iconText: "\u25CE" }
        ListElement { sourceValue: 3; labelText: "CD";        iconText: "\u25CF" }
        ListElement { sourceValue: 4; labelText: "Computer";  iconText: "\u2338" }
        ListElement { sourceValue: 5; labelText: "Bluetooth"; iconText: "\u2261" }
        ListElement { sourceValue: 6; labelText: "Library";   iconText: "\u2630" }
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
        interval: 40   // ~25fps update for smooth ring animation
        repeat: true
        onTriggered: {
            autoSelectProgress += (40 / 4000)  // 4 seconds total
            if (autoSelectProgress >= 1.0) {
                autoSelectTimer.stop()
                autoSelectProgress = 1.0
                ReceiverController.selectInput(indexToSource(focusedIndex))
                carouselOverlay.hide()
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

    // Public show() function — called by main.qml left panel on tap
    function show() {
        carouselOverlay.show()
    }

    // === UIState signal connections (encoder events) ===
    Connections {
        target: UIState
        function onInputNextRequested() {
            carouselOverlay.show()
            carousel.incrementCurrentIndex()
            root.focusInput(carousel.currentIndex)
        }
        function onInputPreviousRequested() {
            carouselOverlay.show()
            carousel.decrementCurrentIndex()
            root.focusInput(carousel.currentIndex)
        }
        function onInputSelectRequested() {
            if (!carouselOverlay.visible) {
                carouselOverlay.show()
            } else {
                root.selectImmediately(carousel.currentIndex)
                carouselOverlay.hide()
            }
        }
    }

    // Sync carousel position with external input changes
    Connections {
        target: ReceiverState
        function onCurrentInputChanged() {
            var idx = root.sourceToIndex(ReceiverState.currentInput)
            if (idx >= 0) {
                root.focusedIndex = idx
                root.autoSelectProgress = 0.0
                autoSelectTimer.stop()
                carousel.currentIndex = idx
            }
        }
    }

    // === Full-screen overlay ===
    Rectangle {
        id: carouselOverlay
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.70)
        visible: false
        opacity: 0.0

        Behavior on opacity {
            NumberAnimation { duration: Theme.animMedium; easing.type: Easing.InOutQuad }
        }

        function show() {
            visible = true
            opacity = 1.0
        }

        function hide() {
            autoSelectTimer.stop()
            root.autoSelectProgress = 0.0
            opacity = 0.0
            hideTimer.restart()
        }

        // Delay setting visible=false until fade-out animation completes
        Timer {
            id: hideTimer
            interval: Theme.animMedium
            onTriggered: carouselOverlay.visible = false
        }

        // Backdrop tap dismisses the carousel
        MouseArea {
            anchors.fill: parent
            onClicked: carouselOverlay.hide()
        }

        // === Horizontal PathView carousel ===
        PathView {
            id: carousel
            anchors.centerIn: parent
            width: parent.width
            height: 380
            model: sourceModel
            snapMode: PathView.SnapToItem
            preferredHighlightBegin: 0.5
            preferredHighlightEnd: 0.5
            highlightRangeMode: PathView.StrictlyEnforceRange
            pathItemCount: sourceModel.count
            interactive: true
            currentIndex: root.focusedIndex >= 0 ? root.focusedIndex : 0

            onMovementStarted: autoSelectTimer.stop()
            onMovementEnded: {
                // Restart auto-select timer after user finishes dragging
                root.focusInput(carousel.currentIndex)
            }
            onCurrentIndexChanged: {
                root.focusInput(currentIndex)
            }

            path: Path {
                startX: -140
                startY: carousel.height / 2
                PathLine {
                    x: 2060
                    y: carousel.height / 2
                }
            }

            delegate: Item {
                id: delegateItem
                width: 220
                height: 380

                required property int index
                required property string labelText
                required property string iconText
                required property int sourceValue

                property bool isCurrent: ReceiverState.currentInput === root.indexToSource(index)
                property bool isFocused: root.focusedIndex === index

                // Distance from center for depth effect
                property real distFromCenter: {
                    var n = sourceModel.count           // 6
                    var offset = PathView.view !== null ? PathView.view.offset : 0
                    var raw = index - (n - offset) % n
                    if (raw > n / 2)  raw -= n
                    if (raw < -n / 2) raw += n
                    return Math.abs(raw)
                }

                property real itemScale:   distFromCenter < 0.5 ? 1.0 : distFromCenter < 1.5 ? 0.72 : 0.45
                property real itemOpacity: distFromCenter < 0.5 ? 1.0 : distFromCenter < 1.5 ? 0.55 : 0.25

                scale:   itemScale
                opacity: itemOpacity

                Behavior on scale {
                    NumberAnimation { duration: Theme.animMedium; easing.type: Easing.OutCubic }
                }
                Behavior on opacity {
                    NumberAnimation { duration: Theme.animMedium; easing.type: Easing.OutCubic }
                }

                Column {
                    anchors.centerIn: parent
                    spacing: Theme.spacingMedium

                    // Icon circle with progress ring
                    Item {
                        width: 96
                        height: 96
                        anchors.horizontalCenter: parent.horizontalCenter

                        // Progress ring (auto-select countdown)
                        Canvas {
                            id: progressRing
                            anchors.fill: parent
                            anchors.margins: -4
                            width: 104
                            height: 104
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
                            width: 96
                            height: 96
                            radius: 48
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
                                font.pixelSize: 36
                                color: Theme.textPrimary
                            }
                        }
                    }

                    // Source label
                    Text {
                        text: delegateItem.labelText
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMedium
                        font.bold: delegateItem.isCurrent
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                }

                // Tap to select immediately and close
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.selectImmediately(delegateItem.index)
                        carouselOverlay.hide()
                    }
                }
            }
        }
    }
}
