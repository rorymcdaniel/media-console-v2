import QtQuick
import QtQuick.Controls
import MediaConsole 1.0

Window {
    id: root
    width: 1920
    height: 720
    visible: true
    visibility: Window.FullScreen
    title: "Media Console"
    color: Theme.primaryBg

    property bool showLibraryBrowser: false
    property bool ejectConfirmVisible: false

    // Global touch activity detection (UI-18)
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

    // ========== Top Status Bar (transparent spacer) ==========
    Rectangle {
        id: statusBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: Theme.statusBarHeight
        color: "transparent"
    }

    // ========== Floating Status Bar Controls ==========

    // Input indicator button (top-left)
    Rectangle {
        id: inputIndicator
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: Theme.spacingSmall
        anchors.leftMargin: Theme.spacingLarge
        width: 180; height: 52; z: 10
        color: Theme.secondaryBg
        radius: Theme.radiusMedium
        border.color: Theme.accent; border.width: 1

        Image {
            source: "qrc:/MediaConsole/icons/input.png"
            width: 40; height: 40
            fillMode: Image.PreserveAspectFit
            smooth: true
            anchors.centerIn: parent
        }
        Rectangle {
            anchors.fill: parent; radius: Theme.radiusMedium
            color: Theme.accent
            opacity: inputIndicatorMa.containsMouse ? 0.15 : 0.0
            Behavior on opacity { NumberAnimation { duration: Theme.animFast } }
        }
        MouseArea {
            id: inputIndicatorMa; anchors.fill: parent
            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
            onClicked: inputCarousel.show()
        }
    }

    // Eject button (top-left, right of InputIndicator)
    Rectangle {
        id: ejectBtn
        anchors.top: parent.top
        anchors.left: inputIndicator.right
        anchors.topMargin: Theme.spacingSmall
        anchors.leftMargin: Theme.spacingSmall
        width: Theme.touchTargetSmall; height: Theme.touchTargetSmall
        radius: Theme.radiusSmall; z: 10
        color: ejectMa.pressed ? Theme.glassBg : "transparent"
        visible: PlaybackState.activeSource === MediaSource.CD

        Text { anchors.centerIn: parent; text: "\u23CF"; color: Theme.textPrimary; font.pixelSize: Theme.fontSizeMedium }

        MouseArea {
            id: ejectMa; anchors.fill: parent
            onClicked: {
                if (PlaybackState.playbackMode === PlaybackMode.Playing) { ejectConfirmVisible = true }
                else { CdController.eject() }
            }
        }
    }

    // Time display (top-center)
    Text {
        id: timeDisplay
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: Theme.spacingSmall + 4
        color: Theme.textSecondary; font.pixelSize: Theme.fontSizeBody; z: 10

        Timer {
            interval: 60000; running: true; repeat: true; triggeredOnStart: true
            onTriggered: timeDisplay.text = Qt.formatTime(new Date(), "h:mm AP")
        }
    }

    // Volume slider row (top-right)
    Row {
        id: volumeRow
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: Theme.spacingSmall
        anchors.rightMargin: Theme.spacingLarge
        spacing: Theme.spacingSmall; z: 10

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

    // Mute button (top-right, left of volumeRow)
    Rectangle {
        id: muteBtn
        anchors.top: parent.top
        anchors.right: volumeRow.left
        anchors.topMargin: Theme.spacingSmall
        anchors.rightMargin: Theme.spacingSmall
        width: Theme.touchTargetSmall; height: Theme.touchTargetSmall
        radius: Theme.radiusSmall; z: 10
        color: ReceiverState.muted ? Qt.rgba(0.906, 0.298, 0.235, 0.3) : "transparent"

        Text { anchors.centerIn: parent; text: ReceiverState.muted ? "\u2716" : "\u266B"; color: ReceiverState.muted ? Theme.errorColor : Theme.textPrimary; font.pixelSize: Theme.fontSizeMedium }
        MouseArea { anchors.fill: parent; onClicked: ReceiverController.toggleMute() }
    }

    // Search button (top-right, left of mute, Streaming only)
    Rectangle {
        id: searchBtn
        anchors.top: parent.top
        anchors.right: muteBtn.left
        anchors.topMargin: Theme.spacingSmall
        anchors.rightMargin: Theme.spacingSmall
        width: Theme.touchTargetSmall; height: Theme.touchTargetSmall
        radius: Theme.radiusSmall; z: 10
        color: searchMa.pressed ? Theme.glassBg : "transparent"
        visible: ReceiverState.currentInput === MediaSource.Streaming

        Text { anchors.centerIn: parent; text: "\u2315"; color: Theme.textPrimary; font.pixelSize: Theme.fontSizeMedium }
        MouseArea { id: searchMa; anchors.fill: parent; onClicked: spotifySearch.visible = true }
    }

    // Connection dot (top-right, left of search)
    Rectangle {
        id: connDot
        anchors.top: parent.top
        anchors.right: searchBtn.left
        anchors.topMargin: Theme.spacingSmall + 10
        anchors.rightMargin: Theme.spacingSmall
        width: 8; height: 8; radius: 4; z: 10
        color: UIState.receiverConnected ? Theme.successColor : Theme.errorColor
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
        anchors.margins: Theme.spacingLarge
        spacing: Theme.spacingLarge

        // Left panel: 1/4 width, rounded, PNG icons
        Rectangle {
            id: leftPanel
            width: (parent.width - parent.spacing) / 4
            height: parent.height
            color: Theme.secondaryBg
            radius: Theme.radiusLarge

            function getServiceIcon() {
                if (ReceiverState.currentInput === MediaSource.Library || root.showLibraryBrowser)
                    return "qrc:/MediaConsole/icons/music-note.svg"
                switch (ReceiverState.currentInput) {
                    case MediaSource.Phono:     return "qrc:/MediaConsole/icons/phono.png"
                    case MediaSource.Bluetooth: return "qrc:/MediaConsole/icons/bluetooth.png"
                    case MediaSource.CD:        return "qrc:/MediaConsole/icons/compact-disc.png"
                    case MediaSource.Computer:  return "qrc:/MediaConsole/icons/computer.png"
                    case MediaSource.Streaming: return "qrc:/MediaConsole/icons/spotify.png"
                    default:                    return ""
                }
            }

            function getDisplayText() {
                if (ReceiverState.currentInput === MediaSource.Library || root.showLibraryBrowser)
                    return "Library"
                switch (ReceiverState.currentInput) {
                    case MediaSource.Streaming:  return "Streaming"
                    case MediaSource.Phono:      return "Phono"
                    case MediaSource.CD:         return "CD"
                    case MediaSource.Computer:   return "Computer"
                    case MediaSource.Bluetooth:  return "Bluetooth"
                    default:                     return "Unknown"
                }
            }

            Column {
                anchors.centerIn: parent
                spacing: Theme.spacingMedium

                Image {
                    source: leftPanel.getServiceIcon()
                    visible: source !== "" && status === Image.Ready
                    width: 256; height: 256
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                Text {
                    text: leftPanel.getDisplayText()
                    font.pixelSize: 64
                    font.bold: true
                    color: Theme.textPrimary
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (ReceiverState.currentInput === MediaSource.Library) {
                        root.showLibraryBrowser = !root.showLibraryBrowser
                    }
                }
            }
        }

        // Right panel: rounded rect with NowPlaying + LibraryBrowser
        Rectangle {
            id: rightPanel
            width: (parent.width - parent.spacing) * 3 / 4
            height: parent.height
            color: Theme.secondaryBg
            radius: Theme.radiusLarge

            NowPlaying {
                id: nowPlaying
                anchors.fill: parent
                anchors.margins: Theme.spacingLarge
                visible: !root.showLibraryBrowser
            }

            LibraryBrowser {
                id: libraryBrowser
                anchors.fill: parent
                anchors.margins: Theme.spacingLarge
                visible: root.showLibraryBrowser
            }
        }
    }

    // ========== Input Carousel Overlay (UI-05) ==========
    InputCarousel {
        id: inputCarousel
        anchors.fill: parent
        z: 500
    }

    // ========== Spotify Search Overlay ==========
    SpotifySearch {
        id: spotifySearch
        anchors.fill: parent
        visible: false
        z: 1000
    }

    // ========== Connections wiring ==========

    // Carousel → library browser
    Connections {
        target: inputCarousel
        function onLibraryRequested() { root.showLibraryBrowser = true }
        function onInputSelected()    { root.showLibraryBrowser = false }
    }

    // Library browser → back to now playing
    Connections {
        target: libraryBrowser
        function onTrackSelected() { root.showLibraryBrowser = false }
    }

    // Input change → hide library if leaving Library input
    Connections {
        target: ReceiverState
        function onCurrentInputChanged() {
            if (ReceiverState.currentInput !== MediaSource.Library) {
                root.showLibraryBrowser = false
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
