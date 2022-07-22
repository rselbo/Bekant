import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material 2.0

import Bekant 1.0

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Bekant desktop controller")

    Material.theme: Material.Dark

    Rectangle {
        id: view
        anchors.fill: parent
        color: Values.backgroundColor

        Item {
            id: adress
            anchors.top: view.top
            anchors.left: view.left
            anchors.right: view.right
            anchors.margins: 10
            height: 20

            BekantInput {
                id: addressEdit
                anchors.left: parent.left
                anchors.right: connectButton.left
                anchors.verticalCenter: parent.verticalCenter
                text: bekant.address
                labelText: "Address"
                onEditingFinished: bekant.addressEdited(addressEdit.text)
            }

            BekantButton {
                id: connectButton
                text: "Connect"
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                onClicked: bekant.onConnect()
                visible: !bekant.connected
            }

            BekantButton {
                id: disconnectButton
                text: "Disconnect"
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                onClicked: bekant.onDisconnect()
                visible: bekant.connected
            }
        }

        Rectangle {
            id: scrollview
            anchors.top: adress.bottom
            anchors.left: parent.left
            anchors.right: memory.left
            anchors.bottom: parent.bottom
            anchors.margins: Values.baseMargin
            border.color: Values.borderColor
            border.width: Values.borderWidth
            color: Values.backgroundColor

            ScrollView {
                id: scroll
                anchors.fill: parent
                anchors.margins: Values.baseMargin
                ScrollBar.vertical.position: 1.0

                TextArea {
                    id: textArea
                    text: bekant.text
                    anchors.fill: parent
                    readOnly: true
                    onTextChanged: textArea.cursorPosition = text.length
                }
            }

            Text {
                id: position1
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.rightMargin: Values.baseMargin
                font.pixelSize: 15
                text: "<b>position1: </b> " + bekant.position1
                color: Values.textColor
            }
            Text {
                anchors.top: position1.bottom
                anchors.right: parent.right
                anchors.rightMargin: Values.baseMargin
                font.pixelSize: 15
                text: "<b>position2: </b> " + bekant.position2
                color: Values.textColor
            }
        }

        Item {
            id: memory
            width: 200
            anchors.top: adress.bottom
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: Values.baseMargin

            Item {
                id: upBox
                anchors.left: memory.left
                anchors.top: memory.top
                width: 100
                height: 50
                BekantButton {
                    id: upButton
                    width: 50
                    height: 50
                    anchors.horizontalCenter: upBox.horizontalCenter
                    icon.source: "up.png"
                    onPressed: bekant.upPressed()
                    onReleased: bekant.upReleased()
                }
            }
            Item {
                id: downBox
                anchors.right: memory.right
                anchors.top: memory.top
                width: 100
                height: 50
                BekantButton {
                    id: downButton
                    width: 50
                    height: 50
                    anchors.horizontalCenter: downBox.horizontalCenter
                    icon.source: "down.png"
                    onPressed: bekant.downPressed()
                    onReleased: bekant.downReleased()
                }
            }

            ListView {
                id: memoryList
                anchors.left: memory.left
                anchors.right: memory.right
                anchors.top: downBox.bottom
                anchors.bottom: memory.bottom
                anchors.topMargin: Values.baseMargin
                spacing: 5
                model: MemoryPositionModel {
                    list: bekant.memoryPositions
                }
                delegate: BekantMemoryPosition {}
            }
        }

    }
}
