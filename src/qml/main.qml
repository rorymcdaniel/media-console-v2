import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import MediaConsole 1.0

Window {
    id: root
    width: 1920
    height: 720
    visible: true
    title: "Media Console"
    color: Theme.primaryBg

    // Eject confirmation modal visibility (UI-13)
    // Set to true when eject is tapped while CD is playing — shows Yes/No dialog
    property bool ejectConfirmVisible: false

    // Global touch activity detection (UI-18)
    // Covers the entire window, forwards all press events to ScreenTimeoutController
    // without consuming them so child elements still receive input.
    MouseArea {
        anchors.fill: parent
        z: 1000
        propagateComposedEvents: true
        acceptedButtons: Qt.AllButtons
        onPressed: function(mouse) {
            ScreenTimeoutController.activityDetected()
            mouse.accepted = false
        }
        onReleased: function(mouse) { mouse.accepted = false }
        onClicked: function(mouse) { mouse.accepted = false }
    }

    // ========== Top Status Bar (UI-03) ==========
    Rectangle {
        id: statusBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: Theme.statusBarHeight
        color: Theme.secondaryBg
        z: 10

        // Bottom separator line
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: Theme.glassBorder
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.spacingMedium
            anchors.rightMargin: Theme.spacingMedium

            // Left: Connection status indicator
            Row {
                spacing: Theme.spacingSmall
                Layout.fillHeight: true

                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: UIState.receiverConnected ? Theme.successColor : Theme.errorColor
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: UIState.receiverConnected ? "Connected" : "Disconnected"
                    color: UIState.receiverConnected ? Theme.textDimmed : Theme.errorColor
                    font.pixelSize: Theme.fontSizeSmall
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            // Spacer
            Item { Layout.fillWidth: true }

            // Right side controls
            RowLayout {
                spacing: Theme.spacingMedium

                // Eject button (UI-13) — visible only when CD is active source
                Rectangle {
                    width: Theme.touchTargetSmall
                    height: Theme.touchTargetSmall
                    radius: Theme.radiusSmall
                    color: ejectMa.pressed ? Theme.glassBg : "transparent"
                    visible: PlaybackState.activeSource === MediaSource.CD

                    Text {
                        anchors.centerIn: parent
                        text: "\u23CF"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMedium
                    }

                    MouseArea {
                        id: ejectMa
                        anchors.fill: parent
                        onClicked: {
                            // CD eject (UI-13): show confirmation if playing, eject immediately if not
                            if (PlaybackState.playbackMode === PlaybackMode.Playing) {
                                ejectConfirmVisible = true
                            } else {
                                CdController.eject()
                            }
                        }
                    }
                }

                // Search button (UI-14) — visible only on Streaming input
                Rectangle {
                    width: Theme.touchTargetSmall
                    height: Theme.touchTargetSmall
                    radius: Theme.radiusSmall
                    color: searchMa.pressed ? Theme.glassBg : "transparent"
                    visible: ReceiverState.currentInput === MediaSource.Streaming

                    Text {
                        anchors.centerIn: parent
                        text: "\u2315"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMedium
                    }

                    MouseArea {
                        id: searchMa
                        anchors.fill: parent
                        onClicked: UIState.setActiveView(ActiveView.SpotifySearch)
                    }
                }

                // Volume indicator with slider (UI-12)
                Row {
                    spacing: Theme.spacingSmall

                    Slider {
                        id: volumeSlider
                        width: 100
                        height: Theme.touchTargetSmall
                        from: 0
                        to: 200
                        value: ReceiverState.volume
                        onMoved: ReceiverController.setVolume(Math.round(value))

                        background: Rectangle {
                            x: volumeSlider.leftPadding
                            y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                            width: volumeSlider.availableWidth
                            height: 4
                            radius: 2
                            color: Theme.glassBg

                            Rectangle {
                                width: volumeSlider.visualPosition * parent.width
                                height: parent.height
                                radius: 2
                                color: Theme.accent
                            }
                        }

                        handle: Rectangle {
                            x: volumeSlider.leftPadding + volumeSlider.visualPosition * (volumeSlider.availableWidth - width)
                            y: volumeSlider.topPadding + volumeSlider.availableHeight / 2 - height / 2
                            width: 16
                            height: 16
                            radius: 8
                            color: volumeSlider.pressed ? Theme.accentLight : Theme.accent
                        }
                    }

                    Text {
                        text: Math.round(ReceiverState.volume / 2) + "%"
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSmall
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                // Mute button (UI-17)
                Rectangle {
                    width: Theme.touchTargetSmall
                    height: Theme.touchTargetSmall
                    radius: Theme.radiusSmall
                    color: ReceiverState.muted ? Qt.rgba(0.906, 0.298, 0.235, 0.3) : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: ReceiverState.muted ? "\u2716" : "\u266B"
                        color: ReceiverState.muted ? Theme.errorColor : Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMedium
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: ReceiverController.toggleMute()
                    }
                }

                // Power button (UI-17)
                Rectangle {
                    width: Theme.touchTargetSmall
                    height: Theme.touchTargetSmall
                    radius: Theme.radiusSmall
                    color: ReceiverState.powered ? Qt.rgba(0.180, 0.361, 0.541, 0.3) : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "\u23FB"
                        color: ReceiverState.powered ? Theme.accentLight : Theme.textDimmed
                        font.pixelSize: Theme.fontSizeMedium
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: ReceiverController.setPower(!ReceiverState.powered)
                    }
                }

                // Time display (UI-16)
                Text {
                    id: timeDisplay
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeBody

                    Timer {
                        interval: 60000
                        running: true
                        repeat: true
                        triggeredOnStart: true
                        onTriggered: timeDisplay.text = Qt.formatTime(new Date(), "h:mm AP")
                    }
                }
            }
        }
    }

    // ========== Error Banner (UI-15) ==========
    Rectangle {
        id: errorBanner
        anchors.top: statusBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: !UIState.receiverConnected ? 40 : 0
        color: Theme.errorColor
        z: 100
        clip: true

        Behavior on height {
            NumberAnimation { duration: Theme.animMedium; easing.type: Easing.OutCubic }
        }

        Row {
            anchors.centerIn: parent
            spacing: Theme.spacingSmall

            Text {
                text: "\u26A0"
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeBody
            }

            Text {
                text: "Receiver disconnected \u2014 attempting to reconnect..."
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeBody
            }
        }
    }

    // ========== Main Content Area ==========
    Row {
        anchors.top: errorBanner.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        // Left panel: Input carousel (UI-03)
        Rectangle {
            id: leftPanel
            width: Theme.leftPanelWidth
            height: parent.height
            color: Theme.secondaryBg

            // Separator line on right edge
            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 1
                color: Theme.glassBorder
            }

            // Input carousel (UI-05)
            InputCarousel {
                anchors.fill: parent
            }
        }

        // Right panel: Active view (UI-03)
        Item {
            id: rightPanel
            width: parent.width - Theme.leftPanelWidth
            height: parent.height

            // View loader — switches component based on UIState.activeView
            Loader {
                id: viewLoader
                anchors.fill: parent
                source: {
                    switch (UIState.activeView) {
                        case ActiveView.NowPlaying: return "components/NowPlaying.qml"
                        case ActiveView.LibraryBrowser: return "components/LibraryBrowser.qml"
                        case ActiveView.SpotifySearch: return "components/SpotifySearch.qml"
                        default: return "components/NowPlaying.qml"
                    }
                }

                // Smooth crossfade when switching views
                opacity: status === Loader.Ready ? 1.0 : 0.0
                Behavior on opacity {
                    NumberAnimation { duration: Theme.animMedium; easing.type: Easing.OutCubic }
                }
            }
        }
    }

    // ========== Volume Overlay (UI-11) ==========
    Rectangle {
        id: volumeOverlay
        anchors.centerIn: parent
        width: 300
        height: 300
        radius: Theme.radiusLarge
        color: Theme.glassBg
        border.color: Theme.glassBorder
        border.width: 1
        visible: UIState.volumeOverlayVisible
        z: 500
        opacity: visible ? 1.0 : 0.0

        Behavior on opacity {
            NumberAnimation { duration: Theme.animFast; easing.type: Easing.OutCubic }
        }

        Column {
            anchors.centerIn: parent
            spacing: Theme.spacingMedium

            Text {
                text: Math.round(ReceiverState.volume / 2)
                color: Theme.textPrimary
                font.pixelSize: 96
                font.bold: true
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Text {
                text: ReceiverState.muted ? "MUTED" : "VOLUME"
                color: ReceiverState.muted ? Theme.errorColor : Theme.textSecondary
                font.pixelSize: Theme.fontSizeMedium
                anchors.horizontalCenter: parent.horizontalCenter
            }
        }

        Timer {
            id: volumeHideTimer
            interval: 2000
            onTriggered: UIState.setVolumeOverlayVisible(false)
        }

        onVisibleChanged: {
            if (visible) volumeHideTimer.restart()
        }
    }

    // ========== Toast Notification (UI-10) ==========
    Rectangle {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 40
        width: toastText.implicitWidth + 60
        height: 48
        radius: Theme.radiusLarge
        color: {
            switch (UIState.toastType) {
                case "error": return Theme.errorColor
                case "success": return Theme.successColor
                default: return Theme.glassBg
            }
        }
        border.color: Theme.glassBorder
        border.width: UIState.toastType === "info" ? 1 : 0
        visible: UIState.toastVisible
        z: 800
        opacity: visible ? 1.0 : 0.0

        Behavior on opacity {
            NumberAnimation { duration: Theme.animFast; easing.type: Easing.OutCubic }
        }

        Text {
            id: toastText
            anchors.centerIn: parent
            text: UIState.toastMessage
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeBody
        }

        Timer {
            id: toastDismissTimer
            interval: 3000
            onTriggered: UIState.setToastVisible(false)
        }

        Connections {
            target: UIState
            function onShowToast(message, type) {
                UIState.setToastMessage(message)
                UIState.setToastType(type)
                UIState.setToastVisible(true)
                toastDismissTimer.restart()
            }
        }
    }

    // ========== Spotify Takeover Dialog (UI-08) ==========
    Rectangle {
        id: takeoverDialog
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.6)
        visible: false
        z: 600
        opacity: visible ? 1.0 : 0.0
        Behavior on opacity {
            NumberAnimation { duration: Theme.animMedium; easing.type: Easing.OutCubic }
        }

        property string deviceName: ""
        property string trackTitle: ""
        property string artistName: ""

        Connections {
            target: SpotifyController
            function onActiveSessionDetected(device, track, artist) {
                takeoverDialog.deviceName = device
                takeoverDialog.trackTitle = track
                takeoverDialog.artistName = artist
                takeoverDialog.visible = true
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {} // Consume clicks on backdrop
        }

        Rectangle {
            anchors.centerIn: parent
            width: 500
            height: 280
            radius: Theme.radiusLarge
            color: Theme.secondaryBg
            border.color: Theme.glassBorder
            border.width: 1

            Column {
                anchors.centerIn: parent
                spacing: Theme.spacingMedium
                width: parent.width - 60

                Text {
                    text: "Transfer Playback?"
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeLarge
                    font.bold: true
                }

                Text {
                    text: "Currently playing on: " + takeoverDialog.deviceName
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeBody
                    wrapMode: Text.Wrap
                    width: parent.width
                }

                Text {
                    text: takeoverDialog.trackTitle + " \u2014 " + takeoverDialog.artistName
                    color: Theme.textDimmed
                    font.pixelSize: Theme.fontSizeBody
                    wrapMode: Text.Wrap
                    width: parent.width
                }

                Row {
                    spacing: Theme.spacingMedium
                    anchors.horizontalCenter: parent.horizontalCenter

                    Rectangle {
                        width: 150
                        height: Theme.touchTargetLarge
                        radius: Theme.radiusMedium
                        color: cancelTakeoverMa.pressed ? Theme.accent : Theme.glassBg

                        Text {
                            anchors.centerIn: parent
                            text: "Cancel"
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeBody
                        }

                        MouseArea {
                            id: cancelTakeoverMa
                            anchors.fill: parent
                            onClicked: {
                                SpotifyController.cancelTransfer()
                                takeoverDialog.visible = false
                            }
                        }
                    }

                    Rectangle {
                        width: 150
                        height: Theme.touchTargetLarge
                        radius: Theme.radiusMedium
                        color: confirmTakeoverMa.pressed ? Theme.accentLight : Theme.accent

                        Text {
                            anchors.centerIn: parent
                            text: "Transfer"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeBody
                        }

                        MouseArea {
                            id: confirmTakeoverMa
                            anchors.fill: parent
                            onClicked: {
                                SpotifyController.confirmTransfer()
                                takeoverDialog.visible = false
                            }
                        }
                    }
                }
            }
        }
    }

    // ========== Audio Error Dialog (UI-09) ==========
    Rectangle {
        id: audioErrorDialog
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.6)
        visible: UIState.audioError !== ""
        z: 600
        opacity: visible ? 1.0 : 0.0
        Behavior on opacity {
            NumberAnimation { duration: Theme.animMedium; easing.type: Easing.OutCubic }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {} // Consume clicks on backdrop
        }

        Rectangle {
            anchors.centerIn: parent
            width: 450
            height: 220
            radius: Theme.radiusLarge
            color: Theme.secondaryBg
            border.color: Theme.errorColor
            border.width: 2

            Column {
                anchors.centerIn: parent
                spacing: Theme.spacingMedium
                width: parent.width - 60

                Text {
                    text: "Audio Error"
                    color: Theme.errorColor
                    font.pixelSize: Theme.fontSizeLarge
                    font.bold: true
                }

                Text {
                    text: UIState.audioError
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeBody
                    wrapMode: Text.Wrap
                    width: parent.width
                }

                Row {
                    spacing: Theme.spacingMedium
                    anchors.horizontalCenter: parent.horizontalCenter

                    Rectangle {
                        width: 150
                        height: Theme.touchTargetLarge
                        radius: Theme.radiusMedium
                        color: dismissErrorMa.pressed ? Theme.accent : Theme.glassBg

                        Text {
                            anchors.centerIn: parent
                            text: "Dismiss"
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeBody
                        }

                        MouseArea {
                            id: dismissErrorMa
                            anchors.fill: parent
                            onClicked: UIState.setAudioError("")
                        }
                    }

                    Rectangle {
                        width: 150
                        height: Theme.touchTargetLarge
                        radius: Theme.radiusMedium
                        color: retryErrorMa.pressed ? Theme.accentLight : Theme.accent

                        Text {
                            anchors.centerIn: parent
                            text: "Restart Now"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeBody
                        }

                        MouseArea {
                            id: retryErrorMa
                            anchors.fill: parent
                            onClicked: {
                                // AUDIO-07: Pluggable restart action — wired to QCoreApplication::quit() in main.cpp
                                // systemd will restart the process automatically
                                UIState.setAudioError("")
                                UIState.restartRequested()
                            }
                        }
                    }
                }
            }
        }
    }

    // ========== Eject Confirmation Modal (UI-13) ==========
    Rectangle {
        id: ejectConfirmDialog
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.6)
        visible: ejectConfirmVisible
        z: 500
        opacity: visible ? 1.0 : 0.0
        Behavior on opacity {
            NumberAnimation { duration: Theme.animMedium; easing.type: Easing.OutCubic }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {} // Consume clicks on backdrop
        }

        Rectangle {
            anchors.centerIn: parent
            width: 400
            height: 180
            radius: Theme.radiusLarge
            color: Theme.secondaryBg
            border.color: Theme.glassBorder
            border.width: 1

            Column {
                anchors.centerIn: parent
                spacing: Theme.spacingMedium
                width: parent.width - 60

                Text {
                    text: "Eject Disc?"
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeLarge
                    font.bold: true
                }

                Text {
                    text: "Playback is active. Stop and eject the disc?"
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeBody
                    wrapMode: Text.Wrap
                    width: parent.width
                }

                Row {
                    spacing: Theme.spacingMedium
                    anchors.horizontalCenter: parent.horizontalCenter

                    Rectangle {
                        width: 150
                        height: Theme.touchTargetLarge
                        radius: Theme.radiusMedium
                        color: cancelEjectMa.pressed ? Theme.accent : Theme.glassBg

                        Text {
                            anchors.centerIn: parent
                            text: "Cancel"
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeBody
                        }

                        MouseArea {
                            id: cancelEjectMa
                            anchors.fill: parent
                            onClicked: ejectConfirmVisible = false
                        }
                    }

                    Rectangle {
                        width: 150
                        height: Theme.touchTargetLarge
                        radius: Theme.radiusMedium
                        color: confirmEjectMa.pressed ? Theme.accentLight : Theme.accent

                        Text {
                            anchors.centerIn: parent
                            text: "Eject"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeBody
                        }

                        MouseArea {
                            id: confirmEjectMa
                            anchors.fill: parent
                            onClicked: {
                                ejectConfirmVisible = false
                                CdController.eject()
                            }
                        }
                    }
                }
            }
        }
    }
}
