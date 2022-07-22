pragma Singleton

import QtQuick 2.3

QtObject {
    readonly property int baseMargin: 5
    readonly property string borderColor: "gray"
    readonly property int borderWidth: 1
    readonly property int standardWidth: 50
    readonly property int standardHeight: 32

    readonly property color backgroundColor: "#454545"
    readonly property color textColor: "white"
}
