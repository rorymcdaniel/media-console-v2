import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import MediaConsole 1.0

Window {
    width: 1920
    height: 720
    visible: true
    title: "Media Console v2 - State Test Harness"
    color: "#0a1628"

    Column {
        anchors.fill: parent
        anchors.margins: 30
        spacing: 20

        Text {
            text: "Media Console v2 - State Test Harness"
            color: "#e0e0e0"
            font.pixelSize: 28
            font.bold: true
        }

        Row {
            spacing: 40
            width: parent.width

            Column {
                spacing: 6
                width: parent.width / 3 - 40

                Text { text: "=== ReceiverState ==="; color: "#e0e0e0"; font.pixelSize: 20; font.bold: true }
                Text { text: "Volume: " + ReceiverState.volume; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Powered: " + ReceiverState.powered; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Muted: " + ReceiverState.muted; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Input: " + ReceiverState.currentInput; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Title: " + ReceiverState.title; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Artist: " + ReceiverState.artist; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Album: " + ReceiverState.album; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Art URL: " + ReceiverState.albumArtUrl; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "File Info: " + ReceiverState.fileInfo; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Service: " + ReceiverState.serviceName; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Streaming: " + ReceiverState.streamingService; color: "#e0e0e0"; font.pixelSize: 16 }
            }

            Column {
                spacing: 6
                width: parent.width / 3 - 40

                Text { text: "=== PlaybackState ==="; color: "#e0e0e0"; font.pixelSize: 20; font.bold: true }
                Text { text: "Mode: " + PlaybackState.playbackMode; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Source: " + PlaybackState.activeSource; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Position: " + PlaybackState.positionMs + " ms"; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Duration: " + PlaybackState.durationMs + " ms"; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Title: " + PlaybackState.title; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Artist: " + PlaybackState.artist; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Album: " + PlaybackState.album; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Art URL: " + PlaybackState.albumArtUrl; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Track: " + PlaybackState.trackNumber + "/" + PlaybackState.trackCount; color: "#e0e0e0"; font.pixelSize: 16 }
            }

            Column {
                spacing: 6
                width: parent.width / 3 - 40

                Text { text: "=== UIState ==="; color: "#e0e0e0"; font.pixelSize: 20; font.bold: true }
                Text { text: "View: " + UIState.activeView; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Volume Overlay: " + UIState.volumeOverlayVisible; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Error Banner: " + UIState.errorBannerVisible; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Toast Visible: " + UIState.toastVisible; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Toast Msg: " + UIState.toastMessage; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Toast Type: " + UIState.toastType; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Connected: " + UIState.receiverConnected; color: "#e0e0e0"; font.pixelSize: 16 }
                Text { text: "Audio Error: " + UIState.audioError; color: "#e0e0e0"; font.pixelSize: 16 }
            }
        }
    }
}
