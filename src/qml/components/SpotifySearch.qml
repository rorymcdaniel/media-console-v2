import QtQuick
import MediaConsole 1.0

Item {
    id: spotifySearch

    property string searchQuery: ""

    onSearchQueryChanged: {
        if (searchQuery.length > 0)
            SpotifyController.search(searchQuery)
        else
            SpotifyController.clearSearch()
    }

    // Background fill
    Rectangle {
        anchors.fill: parent
        color: Theme.primaryBg
    }

    // Close button (top-right)
    Rectangle {
        id: closeBtn
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: Theme.spacingMedium
        width: Theme.touchTargetSmall
        height: Theme.touchTargetSmall
        radius: Theme.touchTargetSmall / 2
        color: closeMa.pressed ? Theme.accent : Theme.glassBg
        border.color: Theme.glassBorder
        border.width: 1
        z: 10

        Text {
            anchors.centerIn: parent
            text: "\u2715"
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeMedium
        }

        MouseArea {
            id: closeMa
            anchors.fill: parent
            onClicked: UIState.setActiveView(ActiveView.NowPlaying)
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: Theme.spacingMedium
        spacing: Theme.spacingMedium

        // ========== Search text field ==========
        Rectangle {
            width: parent.width
            height: Theme.touchTargetLarge
            radius: Theme.radiusMedium
            color: Theme.glassBg
            border.color: Theme.glassBorder
            border.width: 1

            Row {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingMedium
                anchors.rightMargin: Theme.spacingMedium
                spacing: Theme.spacingSmall

                // Search icon
                Text {
                    text: "\u2315"
                    color: Theme.textDimmed
                    font.pixelSize: Theme.fontSizeMedium
                    anchors.verticalCenter: parent.verticalCenter
                }

                // Query text with blinking cursor
                Item {
                    width: parent.width - Theme.fontSizeMedium - Theme.spacingSmall
                    height: parent.height

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: searchQuery.length > 0 ? searchQuery : "Search Spotify..."
                        color: searchQuery.length > 0 ? Theme.textPrimary : Theme.textDimmed
                        font.pixelSize: Theme.fontSizeMedium
                        elide: Text.ElideRight
                        width: parent.width - cursorRect.width
                    }

                    // Blinking cursor
                    Rectangle {
                        id: cursorRect
                        anchors.verticalCenter: parent.verticalCenter
                        x: cursorMetrics.advanceWidth + Theme.spacingSmall
                        width: 2
                        height: Theme.fontSizeMedium + 4
                        color: Theme.accent
                        visible: searchQuery.length > 0
                        opacity: cursorAnim.running ? 1.0 : 0.0

                        SequentialAnimation on opacity {
                            id: cursorAnim
                            running: searchQuery.length > 0
                            loops: Animation.Infinite
                            NumberAnimation { to: 1.0; duration: 0 }
                            PauseAnimation { duration: 500 }
                            NumberAnimation { to: 0.0; duration: 0 }
                            PauseAnimation { duration: 500 }
                        }

                        TextMetrics {
                            id: cursorMetrics
                            text: searchQuery
                            font.pixelSize: Theme.fontSizeMedium
                        }
                    }
                }
            }
        }

        // ========== Search Results ==========
        ListView {
            id: searchResultsList
            width: parent.width
            height: parent.height - Theme.touchTargetLarge - keyboardArea.height - Theme.spacingMedium * 3
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            model: {
                var results = SpotifyController.searchResults
                if (!results || !results.tracks || !results.tracks.items)
                    return []
                return results.tracks.items
            }

            // Empty state
            Text {
                anchors.centerIn: parent
                visible: searchResultsList.count === 0 && searchQuery.length === 0
                text: "Type to search Spotify"
                color: Theme.textDimmed
                font.pixelSize: Theme.fontSizeMedium
            }

            Text {
                anchors.centerIn: parent
                visible: searchResultsList.count === 0 && searchQuery.length > 0
                text: "No results found"
                color: Theme.textDimmed
                font.pixelSize: Theme.fontSizeBody
            }

            // Section header for tracks
            header: Rectangle {
                width: searchResultsList.width
                height: searchResultsList.count > 0 ? 40 : 0
                color: "transparent"
                visible: searchResultsList.count > 0

                Text {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Tracks"
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSmall
                    font.bold: true
                }
            }

            delegate: Rectangle {
                width: searchResultsList.width
                height: 72
                color: resultMa.pressed ? Theme.glassBg : "transparent"

                // Bottom separator
                Rectangle {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 76
                    height: 1
                    color: Theme.glassBorder
                    opacity: 0.5
                }

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingMedium
                    anchors.rightMargin: Theme.spacingMedium
                    spacing: Theme.spacingMedium

                    // Album art thumbnail
                    Rectangle {
                        width: 60
                        height: 60
                        radius: Theme.radiusSmall
                        color: Theme.secondaryBg
                        anchors.verticalCenter: parent.verticalCenter
                        clip: true

                        Image {
                            anchors.fill: parent
                            source: {
                                if (modelData && modelData.album && modelData.album.images) {
                                    var images = modelData.album.images
                                    // Use smallest adequate image (last in array is typically smallest)
                                    if (images.length > 0)
                                        return images[images.length - 1].url || ""
                                }
                                return ""
                            }
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            sourceSize.width: 64
                            sourceSize.height: 64
                        }
                    }

                    // Track info
                    Column {
                        width: parent.width - 60 - typeIndicator.width - Theme.spacingMedium * 3
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 2

                        Text {
                            text: modelData ? (modelData.name || "") : ""
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeBody
                            width: parent.width
                            elide: Text.ElideRight
                        }

                        Text {
                            text: {
                                if (modelData && modelData.artists && modelData.artists.length > 0)
                                    return modelData.artists[0].name || ""
                                return ""
                            }
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeSmall
                            width: parent.width
                            elide: Text.ElideRight
                        }

                        Text {
                            text: {
                                if (modelData && modelData.album)
                                    return modelData.album.name || ""
                                return ""
                            }
                            color: Theme.textDimmed
                            font.pixelSize: Theme.fontSizeSmall
                            width: parent.width
                            elide: Text.ElideRight
                        }
                    }

                    // Type indicator / duration
                    Text {
                        id: typeIndicator
                        text: {
                            if (modelData && modelData.duration_ms) {
                                var totalSec = Math.floor(modelData.duration_ms / 1000)
                                var mins = Math.floor(totalSec / 60)
                                var secs = totalSec % 60
                                return mins + ":" + (secs < 10 ? "0" : "") + secs
                            }
                            return ""
                        }
                        color: Theme.textDimmed
                        font.pixelSize: Theme.fontSizeSmall
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }

                MouseArea {
                    id: resultMa
                    anchors.fill: parent
                    onClicked: {
                        if (modelData && modelData.uri)
                            SpotifyController.playTrackUri(modelData.uri)
                    }
                }
            }
        }

        // ========== On-screen Keyboard ==========
        SimpleKeyboard {
            id: keyboardArea
            width: parent.width
            height: 220

            onKeyPressed: function(key) {
                if (key === "backspace")
                    spotifySearch.searchQuery = spotifySearch.searchQuery.slice(0, -1)
                else
                    spotifySearch.searchQuery += key
            }
        }
    }
}
