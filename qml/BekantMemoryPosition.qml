import QtQuick
import QtQuick.Controls

Component {
    id: contactDelegate
    Rectangle {
        id: root
        width: ListView.view.width
        height: 50

        border.color: Values.borderColor
        border.width: Values.borderWidth
        color: Values.backgroundColor
        radius: 10

        Image {
            anchors.fill: parent
            source: "listbackground.png"
        }
        Item {
            anchors.fill: parent
            anchors.leftMargin: Values.baseMargin
            TextInput {
                id: name
                font.pixelSize: 30
                text: model.name
                anchors.verticalCenter: parent.verticalCenter
                readOnly: true
                color: model.position.length > 0 ? Values.textColor : "#cccccc"
                onEditingFinished: {
                    if(text === "") {
                        text = model.name
                        mouseArea.enabled = true;
                        readOnly = true;
                        cursorVisible = false;
                        focus = false;
                        return;
                    }
                    model.name = text;
                    model.position = bekant.position
                    mouseArea.enabled = true;
                    readOnly = true;
                    cursorVisible = false;
                    focus = false;
                }
            }
        }

        Item {
            anchors.fill: parent
            anchors.leftMargin: Values.baseMargin

            Text {
                font.pixelSize: 25
                text: model.position
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                color: Values.textColor
            }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: (mouse) => {
               if(mouse.button === Qt.LeftButton) {
                   if(model.position === "") {
                       mouseArea.enabled = false;
                       name.text = "";
                       name.readOnly = false;
                       name.forceActiveFocus();
                       return;
                   }
                   bekant.goToPosition(model.position);

                   return;
               }
               if(mouse.button === Qt.RightButton) {
                   contextMenu.popup();
                   return;
               }
            }
            Menu {
                id: contextMenu
                MenuItem {
                    text: "Rename"
                    onTriggered: {
                        mouseArea.enabled = false;
                        name.readOnly = false;
                        name.forceActiveFocus()
                    }
                }
                MenuItem {
                    text: "Update Position"
                    onTriggered: {
                        model.position = bekant.position
                    }
                }
                MenuItem {
                    text: "Delete"
                    onTriggered: memoryList.model.deleteMemoryPosition(index)
                }
            }
        }
    }
}
