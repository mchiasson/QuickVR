import QtQuick 2.15
import QuickVR 1.0

VRWindow {
    visible: true
    width: 640
    height: 480
    title: qsTr("RoomTiny")
    color: "black"

    VRHeadset {
        id: headset
        x:  0
        y:  0
        z: -5
//        transform: Rotation {
//            axis { x: 0; y: 1; z: 0 }
//            angle: 180
//        }
        rotation: 180;

        focus: true
        Keys.onPressed: handleKeyEvent(event, true);
        Keys.onReleased: handleKeyEvent(event, false);

        function handleKeyEvent(event, pressed)
        {
            if (!event.isAutoRepeat) {
                if (event.key === Qt.Key_Left)
                {
                    angularVelocity.y += pressed ? 0.75 : -0.75;
                    event.accepted = true;
                }
                else if (event.key === Qt.Key_Right)
                {
                    angularVelocity.y += pressed ? -0.75 : 0.75;
                    event.accepted = true;
                }
                else if (event.key === Qt.Key_W || event.key === Qt.Key_Up)
                {
                    linearVelocity.z += pressed ? -0.05 : 0.05;
                    event.accepted = true;
                }
                else if (event.key === Qt.Key_S || event.key === Qt.Key_Down)
                {
                    linearVelocity.z += pressed ? 0.05 : -0.05;
                    event.accepted = true;
                }
                else if (event.key === Qt.Key_A)
                {
                    linearVelocity.x += pressed ? -0.05 : 0.05;
                    event.accepted = true;
                }
                else if (event.key === Qt.Key_D)
                {
                    linearVelocity.x += pressed ? 0.05 : -0.05;
                    event.accepted = true;
                }
            }
        }
    }
}
