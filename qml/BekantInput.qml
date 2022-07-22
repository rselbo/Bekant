import QtQuick
import QtQuick.Controls

Rectangle {
    property alias text: input.text
    property alias labelText: label.text
    signal editingFinished()

    height: Values.standardHeight
    color: Values.backgroundColor

    Label {
        id: label
        visible: (text.length > 0)
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
    }
    Rectangle {
        id: inputRect

        anchors.left: label.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: Values.baseMargin

        border.color: Values.borderColor
        border.width: Values.borderWidth
        color: Values.backgroundColor

        TextInput {
            id: input
            anchors.fill: parent
            anchors.margins: Values.baseMargin
            onEditingFinished: inputRect.editingFinished()
            text: ""
            color: Values.textColor
        }
    }
}
