import QtQuick
import QtQuick.Controls
import MediaConsole 1.0

Item {
    id: libraryBrowser

    signal trackSelected()

    StackView {
        id: libraryStack
        anchors.fill: parent
        initialItem: artistListComponent

        // Slide transitions for push/pop
        pushEnter: Transition {
            PropertyAnimation {
                property: "x"
                from: libraryStack.width
                to: 0
                duration: Theme.animMedium
                easing.type: Easing.OutCubic
            }
        }
        pushExit: Transition {
            PropertyAnimation {
                property: "x"
                from: 0
                to: -libraryStack.width
                duration: Theme.animMedium
                easing.type: Easing.OutCubic
            }
        }
        popEnter: Transition {
            PropertyAnimation {
                property: "x"
                from: -libraryStack.width
                to: 0
                duration: Theme.animMedium
                easing.type: Easing.OutCubic
            }
        }
        popExit: Transition {
            PropertyAnimation {
                property: "x"
                from: 0
                to: libraryStack.width
                duration: Theme.animMedium
                easing.type: Easing.OutCubic
            }
        }
    }

    // ========== Artist List (initial view) ==========
    Component {
        id: artistListComponent

        Item {
            StackView.onRemoved: destroy()

            Row {
                anchors.fill: parent

                ListView {
                    id: artistList
                    width: parent.width - 30
                    height: parent.height
                    model: LibraryArtistModel
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: Rectangle {
                        width: artistList.width
                        height: Theme.touchTargetLarge
                        color: artistDelegateMa.pressed ? Theme.glassBg : "transparent"

                        // Bottom separator
                        Rectangle {
                            anchors.bottom: parent.bottom
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: Theme.spacingMedium
                            anchors.rightMargin: Theme.spacingMedium
                            height: 1
                            color: Theme.glassBorder
                        }

                        Text {
                            text: albumArtist
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeBody
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: Theme.spacingMedium
                            anchors.right: albumCountText.left
                            anchors.rightMargin: Theme.spacingSmall
                            elide: Text.ElideRight
                        }

                        Text {
                            id: albumCountText
                            text: albumCount + (albumCount === 1 ? " album" : " albums")
                            color: Theme.textDimmed
                            font.pixelSize: Theme.fontSizeSmall
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right
                            anchors.rightMargin: Theme.spacingMedium
                        }

                        MouseArea {
                            id: artistDelegateMa
                            anchors.fill: parent
                            onClicked: {
                                LibraryAlbumModel.artistFilter = albumArtist
                                libraryStack.push(albumGridComponent, { artistName: albumArtist })
                            }
                        }
                    }

                    // Scroll position helper for alphabet sidebar
                    function scrollToLetter(letter) {
                        for (var i = 0; i < LibraryArtistModel.rowCount(); i++) {
                            var name = LibraryArtistModel.data(
                                LibraryArtistModel.index(i, 0),
                                Qt.UserRole + 1  // albumArtist role
                            )
                            if (name === undefined || name === null) continue
                            var firstChar = name.charAt(0).toUpperCase()
                            if (letter === "#" && !/^[A-Z]/.test(firstChar)) {
                                artistList.positionViewAtIndex(i, ListView.Beginning)
                                return
                            }
                            if (firstChar === letter) {
                                artistList.positionViewAtIndex(i, ListView.Beginning)
                                return
                            }
                        }
                    }
                }

                AlphabetSidebar {
                    height: parent.height
                    onLetterSelected: function(letter) {
                        artistList.scrollToLetter(letter)
                    }
                }
            }
        }
    }

    // ========== Album Grid (pushed view) ==========
    Component {
        id: albumGridComponent

        Item {
            property string artistName: ""
            StackView.onRemoved: destroy()

            // Header with back button
            Rectangle {
                id: albumHeader
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: Theme.touchTargetLarge
                color: "transparent"

                Row {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.spacingMedium
                    spacing: Theme.spacingMedium

                    // Back button
                    Rectangle {
                        width: Theme.touchTargetSmall
                        height: Theme.touchTargetSmall
                        radius: Theme.radiusSmall
                        color: backAlbumMa.pressed ? Theme.glassBg : "transparent"
                        anchors.verticalCenter: parent.verticalCenter

                        Text {
                            anchors.centerIn: parent
                            text: "\u2190"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMedium
                        }

                        MouseArea {
                            id: backAlbumMa
                            anchors.fill: parent
                            onClicked: libraryStack.pop()
                        }
                    }

                    Text {
                        text: artistName
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeLarge
                        font.bold: true
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            // Album grid
            GridView {
                id: albumGrid
                anchors.top: albumHeader.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: Theme.spacingMedium
                model: LibraryAlbumModel
                clip: true
                boundsBehavior: Flickable.StopAtBounds

                cellWidth: width / Math.max(1, Math.floor(width / 250))
                cellHeight: cellWidth + 60  // Extra space for text below art

                delegate: Item {
                    width: albumGrid.cellWidth
                    height: albumGrid.cellHeight

                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: Theme.spacingSmall
                        color: albumCellMa.pressed ? Theme.glassBg : "transparent"
                        radius: Theme.radiusSmall

                        Column {
                            anchors.fill: parent
                            spacing: Theme.spacingSmall

                            // Album art
                            Rectangle {
                                width: parent.width
                                height: width
                                radius: Theme.radiusSmall
                                color: Theme.secondaryBg
                                clip: true

                                Image {
                                    anchors.fill: parent
                                    source: artPath ? "file://" + artPath : ""
                                    fillMode: Image.PreserveAspectCrop
                                    asynchronous: true
                                    sourceSize.width: 300
                                    sourceSize.height: 300
                                    visible: status === Image.Ready
                                }

                                // Placeholder when no art
                                Text {
                                    anchors.centerIn: parent
                                    text: "\u266B"
                                    color: Theme.textDimmed
                                    font.pixelSize: Theme.fontSizeHuge
                                    visible: artPath === "" || artPath === undefined
                                }
                            }

                            // Album name
                            Text {
                                text: album
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeSmall
                                width: parent.width
                                horizontalAlignment: Text.AlignHCenter
                                elide: Text.ElideRight
                            }

                            // Year
                            Text {
                                text: year > 0 ? year.toString() : ""
                                color: Theme.textDimmed
                                font.pixelSize: Theme.fontSizeSmall
                                width: parent.width
                                horizontalAlignment: Text.AlignHCenter
                                visible: year > 0
                            }
                        }

                        MouseArea {
                            id: albumCellMa
                            anchors.fill: parent
                            onClicked: {
                                LibraryTrackModel.artistFilter = artistName
                                LibraryTrackModel.albumFilter = album
                                libraryStack.push(trackPageComponent, {
                                    albumTitle: album,
                                    albumArtist: artistName,
                                    albumYear: year,
                                    albumArtPath: artPath || ""
                                })
                            }
                        }
                    }
                }
            }
        }
    }

    // ========== Track Page (pushed view — split layout) ==========
    Component {
        id: trackPageComponent

        Item {
            property string albumTitle: ""
            property string albumArtist: ""
            property int albumYear: 0
            property string albumArtPath: ""
            StackView.onRemoved: destroy()

            // Header with back button
            Rectangle {
                id: trackHeader
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: Theme.touchTargetLarge
                color: "transparent"

                Row {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.spacingMedium
                    spacing: Theme.spacingMedium

                    // Back button
                    Rectangle {
                        width: Theme.touchTargetSmall
                        height: Theme.touchTargetSmall
                        radius: Theme.radiusSmall
                        color: backTrackMa.pressed ? Theme.glassBg : "transparent"
                        anchors.verticalCenter: parent.verticalCenter

                        Text {
                            anchors.centerIn: parent
                            text: "\u2190"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMedium
                        }

                        MouseArea {
                            id: backTrackMa
                            anchors.fill: parent
                            onClicked: libraryStack.pop()
                        }
                    }

                    Text {
                        text: albumTitle
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeLarge
                        font.bold: true
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            // Split layout: left third (art + info), right two-thirds (track list)
            Row {
                anchors.top: trackHeader.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: Theme.spacingMedium
                spacing: Theme.spacingLarge

                // Left column: album art + info
                Column {
                    width: parent.width / 3
                    height: parent.height
                    spacing: Theme.spacingMedium

                    // Album art
                    Rectangle {
                        width: parent.width
                        height: width
                        radius: Theme.radiusMedium
                        color: Theme.secondaryBg
                        clip: true

                        Image {
                            anchors.fill: parent
                            source: albumArtPath ? "file://" + albumArtPath : ""
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            sourceSize.width: 500
                            sourceSize.height: 500
                            visible: status === Image.Ready
                        }

                        // Placeholder
                        Text {
                            anchors.centerIn: parent
                            text: "\u266B"
                            color: Theme.textDimmed
                            font.pixelSize: 72
                            visible: albumArtPath === ""
                        }
                    }

                    // Album info
                    Text {
                        text: albumArtist
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeMedium
                        width: parent.width
                        wrapMode: Text.Wrap
                    }

                    Text {
                        text: albumTitle
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeBody
                        font.bold: true
                        width: parent.width
                        wrapMode: Text.Wrap
                    }

                    Text {
                        text: albumYear > 0 ? albumYear.toString() : ""
                        color: Theme.textDimmed
                        font.pixelSize: Theme.fontSizeSmall
                        visible: albumYear > 0
                    }

                    // Total duration calculated from track model
                    Text {
                        id: totalDurationText
                        color: Theme.textDimmed
                        font.pixelSize: Theme.fontSizeSmall

                        function updateDuration() {
                            var totalSec = 0
                            for (var i = 0; i < LibraryTrackModel.rowCount(); i++) {
                                var dur = LibraryTrackModel.data(
                                    LibraryTrackModel.index(i, 0),
                                    Qt.UserRole + 6  // durationSeconds role
                                )
                                if (dur) totalSec += dur
                            }
                            var mins = Math.floor(totalSec / 60)
                            var secs = totalSec % 60
                            text = LibraryTrackModel.rowCount() + " tracks \u2022 " + mins + ":" + (secs < 10 ? "0" : "") + secs
                        }

                        Component.onCompleted: updateDuration()
                        Connections {
                            target: LibraryTrackModel
                            function onRowsInserted() { totalDurationText.updateDuration() }
                            function onModelReset() { totalDurationText.updateDuration() }
                        }
                    }
                }

                // Right column: track list
                ListView {
                    id: trackListView
                    width: parent.width * 2 / 3 - Theme.spacingLarge
                    height: parent.height
                    model: LibraryTrackModel
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: Rectangle {
                        width: trackListView.width
                        height: Theme.touchTargetLarge
                        color: trackDelegateMa.pressed ? Theme.glassBg : "transparent"

                        // Bottom separator
                        Rectangle {
                            anchors.bottom: parent.bottom
                            anchors.left: parent.left
                            anchors.right: parent.right
                            height: 1
                            color: Theme.glassBorder
                            opacity: 0.5
                        }

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.spacingMedium
                            anchors.rightMargin: Theme.spacingMedium
                            spacing: Theme.spacingMedium

                            // Track number
                            Text {
                                text: trackNumber.toString()
                                color: Theme.textDimmed
                                font.pixelSize: Theme.fontSizeSmall
                                width: 30
                                horizontalAlignment: Text.AlignRight
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            // Title
                            Text {
                                text: title
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeBody
                                width: parent.width - 30 - durationText.width - Theme.spacingMedium * 2
                                elide: Text.ElideRight
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            // Duration (formatted as m:ss)
                            Text {
                                id: durationText
                                text: {
                                    var mins = Math.floor(durationSeconds / 60)
                                    var secs = durationSeconds % 60
                                    return mins + ":" + (secs < 10 ? "0" : "") + secs
                                }
                                color: Theme.textDimmed
                                font.pixelSize: Theme.fontSizeSmall
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        MouseArea {
                            id: trackDelegateMa
                            anchors.fill: parent
                            onClicked: {
                                FlacLibraryController.playTrack(index)
                                libraryBrowser.trackSelected()
                                UIState.setActiveView(ActiveView.NowPlaying)
                            }
                        }
                    }
                }
            }
        }
    }
}
