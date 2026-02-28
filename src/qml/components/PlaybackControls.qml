import QtQuick
import QtQuick.Controls
import MediaConsole 1.0

Item {
    id: root
    implicitHeight: controlRow.implicitHeight

    // Adaptive visibility based on active source (UI-04)
    // CD/Library: previous, play/pause, next
    // Streaming: previous, play/pause, next (shuffle/repeat deferred)
    // Bluetooth: play/pause only
    // Phono/Computer: no playback controls
    property bool showPlayPause: {
        var s = PlaybackState.activeSource
        return s === MediaSource.CD || s === MediaSource.Library ||
               s === MediaSource.Streaming || s === MediaSource.Bluetooth
    }
    property bool showPrevNext: {
        var s = PlaybackState.activeSource
        return s === MediaSource.CD || s === MediaSource.Library || s === MediaSource.Streaming
    }
    property bool showControls: showPlayPause || showPrevNext

    visible: showControls

    Row {
        id: controlRow
        spacing: Theme.spacingLarge
        anchors.centerIn: parent

        // Previous button (not for Bluetooth/Phono/Computer)
        Rectangle {
            width: Theme.touchTargetLarge
            height: Theme.touchTargetLarge
            radius: Theme.touchTargetLarge / 2
            color: previousMa.pressed ? Theme.glassBg : "transparent"
            visible: root.showPrevNext

            Text {
                anchors.centerIn: parent
                text: "\u23EE"
                font.pixelSize: 24
                color: Theme.textPrimary
            }

            MouseArea {
                id: previousMa
                anchors.fill: parent
                onClicked: PlaybackRouter.previous()
            }
        }

        // Play/Pause button (always shown for playback sources)
        Rectangle {
            width: Theme.touchTargetXLarge
            height: Theme.touchTargetXLarge
            radius: Theme.touchTargetXLarge / 2
            color: playMa.pressed ? Theme.accentLight : Theme.dynamicAccent
            visible: root.showPlayPause

            Text {
                anchors.centerIn: parent
                text: PlaybackState.playbackMode === PlaybackMode.Playing ? "\u23F8" : "\u25B6"
                font.pixelSize: 32
                color: Theme.textPrimary
            }

            MouseArea {
                id: playMa
                anchors.fill: parent
                onClicked: {
                    if (PlaybackState.playbackMode === PlaybackMode.Playing) {
                        PlaybackRouter.pause()
                    } else {
                        PlaybackRouter.play()
                    }
                }
            }
        }

        // Next button (not for Bluetooth/Phono/Computer)
        Rectangle {
            width: Theme.touchTargetLarge
            height: Theme.touchTargetLarge
            radius: Theme.touchTargetLarge / 2
            color: nextMa.pressed ? Theme.glassBg : "transparent"
            visible: root.showPrevNext

            Text {
                anchors.centerIn: parent
                text: "\u23ED"
                font.pixelSize: 24
                color: Theme.textPrimary
            }

            MouseArea {
                id: nextMa
                anchors.fill: parent
                onClicked: PlaybackRouter.next()
            }
        }
    }
}
