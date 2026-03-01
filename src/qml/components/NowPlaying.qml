import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MediaConsole 1.0

Item {
    id: root

    // Helper: format milliseconds to "m:ss"
    function formatTime(ms) {
        var s = Math.floor(ms / 1000)
        var m = Math.floor(s / 60)
        s = s % 60
        return m + ":" + (s < 10 ? "0" : "") + s
    }

    // Idle state: nothing playing and no last-played info
    property bool isIdle: PlaybackState.playbackMode === PlaybackMode.Stopped && PlaybackState.title === ""
    // Stopped but has last-played info
    property bool isLastPlayed: PlaybackState.playbackMode === PlaybackMode.Stopped && PlaybackState.title !== ""

    // Main content opacity (dimmed when showing last-played info)
    property real contentOpacity: isIdle ? 0.3 : (isLastPlayed ? 0.4 : 1.0)

    // === Layout: 60/40 split ===
    Row {
        anchors.fill: parent
        anchors.margins: Theme.spacingLarge

        // ========== Left: Album Art (~60%) ==========
        Item {
            id: artContainer
            width: parent.width * 0.6
            height: parent.height
            opacity: root.contentOpacity

            Behavior on opacity {
                NumberAnimation { duration: Theme.animMedium }
            }

            // 3D Flipable for album art (UI-04 flip animation)
            Flipable {
                id: flipable
                anchors.centerIn: parent
                width: Math.min(parent.width - Theme.spacingLarge * 2, parent.height - Theme.spacingLarge * 2)
                height: width

                property bool flipped: false

                front: Item {
                    anchors.fill: parent

                    // Glass frame
                    Rectangle {
                        anchors.fill: parent
                        radius: Theme.radiusMedium
                        color: Theme.glassBg
                        border.color: Theme.glassBorder
                        border.width: 1
                    }

                    // Album art image
                    Image {
                        id: albumArtImage
                        anchors.fill: parent
                        anchors.margins: 2
                        source: AlbumArtResolver.albumArtUrl
                        asynchronous: true
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        visible: status === Image.Ready

                        onStatusChanged: {
                            if (status === Image.Ready) {
                                colorExtractor.extractFrom(albumArtImage)
                            }
                        }
                    }

                    // Placeholder when no art
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 2
                        radius: Theme.radiusMedium - 2
                        color: Theme.primaryBg
                        visible: albumArtImage.status !== Image.Ready

                        Text {
                            anchors.centerIn: parent
                            text: "\u266B"
                            font.pixelSize: 96
                            color: Theme.textDimmed
                            opacity: 0.3
                        }
                    }
                }

                back: Item {
                    anchors.fill: parent

                    // Glass frame (back side)
                    Rectangle {
                        anchors.fill: parent
                        radius: Theme.radiusMedium
                        color: Theme.glassBg
                        border.color: Theme.glassBorder
                        border.width: 1
                    }

                    // Album details on back
                    Column {
                        anchors.centerIn: parent
                        spacing: Theme.spacingMedium
                        width: parent.width - Theme.spacingXLarge * 2

                        Text {
                            text: PlaybackState.album || "Unknown Album"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeLarge
                            font.bold: true
                            wrapMode: Text.Wrap
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Text {
                            text: PlaybackState.artist || "Unknown Artist"
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeMedium
                            wrapMode: Text.Wrap
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                        }

                        Text {
                            text: PlaybackState.trackCount > 0
                                ? PlaybackState.trackCount + " tracks"
                                : ""
                            color: Theme.textDimmed
                            font.pixelSize: Theme.fontSizeBody
                            visible: text !== ""
                            horizontalAlignment: Text.AlignHCenter
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        Text {
                            text: PlaybackState.durationMs > 0
                                ? "Total: " + root.formatTime(PlaybackState.durationMs)
                                : ""
                            color: Theme.textDimmed
                            font.pixelSize: Theme.fontSizeBody
                            visible: text !== ""
                            horizontalAlignment: Text.AlignHCenter
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }

                transform: Rotation {
                    id: flipRotation
                    origin.x: flipable.width / 2
                    origin.y: flipable.height / 2
                    axis { x: 0; y: 1; z: 0 }
                    angle: 0
                }

                states: State {
                    name: "back"
                    when: flipable.flipped
                    PropertyChanges { target: flipRotation; angle: 180 }
                }

                transitions: Transition {
                    NumberAnimation {
                        target: flipRotation
                        property: "angle"
                        duration: Theme.animSlow
                        easing.type: Easing.InOutQuad
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: flipable.flipped = !flipable.flipped
                }
            }
        }

        // ========== Right: Track Info + Controls (~40%) ==========
        Item {
            id: infoContainer
            width: parent.width * 0.4
            height: parent.height
            opacity: root.contentOpacity

            Behavior on opacity {
                NumberAnimation { duration: Theme.animMedium }
            }

            // ========== CD Track List (UI-13) ==========
            // Visible when CD source is active and TOC is available.
            // Positioned at the bottom of the info panel so track info + controls
            // remain visible above it.
            Item {
                id: cdTrackListContainer
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: Theme.spacingMedium
                height: Math.min(parent.height * 0.45, 200)
                visible: PlaybackState.activeSource === MediaSource.CD && CdController.toc.length > 0

                Rectangle {
                    anchors.fill: parent
                    color: Theme.glassBg
                    radius: Theme.radiusMedium
                    border.color: Theme.glassBorder
                    border.width: 1
                }

                ListView {
                    id: trackListView
                    anchors.fill: parent
                    anchors.margins: 4
                    clip: true
                    model: CdController.toc

                    delegate: Item {
                        width: trackListView.width
                        height: Theme.touchTargetSmall

                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 2
                            radius: Theme.radiusSmall
                            color: trackItemMa.pressed
                                   ? Theme.accent
                                   : (PlaybackState.trackNumber === modelData.trackNumber
                                      ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.25)
                                      : "transparent")
                        }

                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: Theme.spacingSmall
                            anchors.rightMargin: Theme.spacingSmall
                            spacing: Theme.spacingSmall

                            Text {
                                text: modelData.trackNumber
                                color: PlaybackState.trackNumber === modelData.trackNumber
                                       ? Theme.dynamicAccent
                                       : Theme.textDimmed
                                font.pixelSize: Theme.fontSizeSmall
                                width: 24
                                horizontalAlignment: Text.AlignRight
                            }

                            Text {
                                text: modelData.title !== "" ? modelData.title : ("Track " + modelData.trackNumber)
                                color: PlaybackState.trackNumber === modelData.trackNumber
                                       ? Theme.textPrimary
                                       : Theme.textSecondary
                                font.pixelSize: Theme.fontSizeBody
                                elide: Text.ElideRight
                                width: parent.width - 24 - Theme.spacingSmall * 2 - durationText.implicitWidth
                            }

                            Text {
                                id: durationText
                                text: {
                                    var s = modelData.durationSeconds
                                    var m = Math.floor(s / 60)
                                    s = s % 60
                                    return m + ":" + (s < 10 ? "0" : "") + s
                                }
                                color: Theme.textDimmed
                                font.pixelSize: Theme.fontSizeSmall
                            }
                        }

                        MouseArea {
                            id: trackItemMa
                            anchors.fill: parent
                            onClicked: CdController.playTrack(modelData.trackNumber)
                        }
                    }
                }
            }

            Item {
                id: infoColumnWrapper
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: Theme.spacingLarge
                anchors.leftMargin: Theme.spacingLarge
                anchors.rightMargin: Theme.spacingLarge
                // When CD track list is visible, leave room for it at the bottom
                anchors.bottom: PlaybackState.activeSource === MediaSource.CD && CdController.toc.length > 0
                                ? cdTrackListContainer.top
                                : parent.bottom

            Column {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: Theme.spacingMedium

                // Idle message
                Text {
                    text: "No music playing"
                    color: Theme.textDimmed
                    font.pixelSize: Theme.fontSizeMedium
                    visible: root.isIdle
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                // Track title
                Text {
                    text: PlaybackState.title || ""
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeLarge
                    font.bold: true
                    wrapMode: Text.Wrap
                    width: parent.width
                    maximumLineCount: 2
                    elide: Text.ElideRight
                    visible: !root.isIdle
                }

                // Artist
                Text {
                    text: PlaybackState.artist || ""
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMedium
                    wrapMode: Text.Wrap
                    width: parent.width
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    visible: !root.isIdle && text !== ""
                }

                // Album
                Text {
                    text: PlaybackState.album || ""
                    color: Theme.textDimmed
                    font.pixelSize: Theme.fontSizeBody
                    wrapMode: Text.Wrap
                    width: parent.width
                    maximumLineCount: 1
                    elide: Text.ElideRight
                    visible: !root.isIdle && text !== ""
                }

                // Track number
                Text {
                    text: "Track " + PlaybackState.trackNumber + "/" + PlaybackState.trackCount
                    color: Theme.textDimmed
                    font.pixelSize: Theme.fontSizeSmall
                    visible: !root.isIdle && PlaybackState.trackCount > 0
                }

                // Spacer
                Item { width: 1; height: Theme.spacingMedium }

                // ========== Progress Bar with Seek ==========
                Column {
                    width: parent.width
                    spacing: Theme.spacingSmall
                    visible: !root.isIdle && PlaybackState.durationMs > 0

                    Slider {
                        id: progressSlider
                        width: parent.width
                        height: Theme.touchTargetSmall
                        from: 0
                        to: PlaybackState.durationMs > 0 ? PlaybackState.durationMs : 1
                        value: progressSlider.pressed ? value : PlaybackState.positionMs
                        onMoved: PlaybackRouter.seek(Math.round(value))

                        background: Rectangle {
                            x: progressSlider.leftPadding
                            y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                            width: progressSlider.availableWidth
                            height: 4
                            radius: 2
                            color: Theme.glassBg

                            Rectangle {
                                width: progressSlider.visualPosition * parent.width
                                height: parent.height
                                radius: 2
                                color: Theme.dynamicAccent
                            }
                        }

                        handle: Rectangle {
                            x: progressSlider.leftPadding + progressSlider.visualPosition * (progressSlider.availableWidth - width)
                            y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                            width: 16
                            height: 16
                            radius: 8
                            color: progressSlider.pressed ? Theme.accentLight : Theme.dynamicAccent
                        }
                    }

                    // Time display: current / total
                    Item {
                        width: parent.width
                        height: currentTimeText.implicitHeight

                        Text {
                            id: currentTimeText
                            anchors.left: parent.left
                            text: root.formatTime(PlaybackState.positionMs)
                            color: Theme.textDimmed
                            font.pixelSize: Theme.fontSizeSmall
                        }

                        Text {
                            id: totalTimeText
                            anchors.right: parent.right
                            text: root.formatTime(PlaybackState.durationMs)
                            color: Theme.textDimmed
                            font.pixelSize: Theme.fontSizeSmall
                        }
                    }
                }

                // Spacer
                Item { width: 1; height: Theme.spacingSmall; visible: !root.isIdle }

                // ========== Adaptive Playback Controls ==========
                PlaybackControls {
                    width: parent.width
                    height: Theme.touchTargetXLarge
                    visible: !root.isIdle
                }
            }
            } // infoColumnWrapper
        }
    }

    // === Dynamic Accent Color Extraction ===
    Canvas {
        id: colorExtractor
        width: 10
        height: 10
        visible: false

        function extractFrom(img) {
            // Schedule a paint to extract the dominant color
            colorExtractor.loadImage(img.source)
        }

        onImageLoaded: {
            requestPaint()
        }

        onPaint: {
            var ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            // Draw the loaded image scaled to 10x10 for color averaging
            if (isImageLoaded(albumArtImage.source)) {
                ctx.drawImage(albumArtImage.source.toString(), 0, 0, 10, 10)
                var data = ctx.getImageData(0, 0, 10, 10).data
                var r = 0, g = 0, b = 0
                var count = data.length / 4
                for (var i = 0; i < data.length; i += 4) {
                    r += data[i]
                    g += data[i + 1]
                    b += data[i + 2]
                }
                // Average and set dynamic accent (ensure minimum brightness)
                var avgR = r / count / 255
                var avgG = g / count / 255
                var avgB = b / count / 255

                // Boost brightness if too dark (minimum luminance threshold)
                var lum = 0.299 * avgR + 0.587 * avgG + 0.114 * avgB
                if (lum < 0.2) {
                    var boost = 0.2 / Math.max(lum, 0.01)
                    avgR = Math.min(avgR * boost, 1.0)
                    avgG = Math.min(avgG * boost, 1.0)
                    avgB = Math.min(avgB * boost, 1.0)
                }

                Theme.dynamicAccent = Qt.rgba(avgR, avgG, avgB, 1.0)
            }
        }
    }

    // Reset dynamic accent when art clears
    Connections {
        target: AlbumArtResolver
        function onAlbumArtUrlChanged() {
            if (AlbumArtResolver.albumArtUrl === "") {
                Theme.dynamicAccent = Theme.accent
            }
        }
    }
}
