import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import QtQuick.Dialogs
import NoteBot 1.0

Window {
    id: root
    visible: true
    width: 960
    height: 640
    minimumWidth: 860
    minimumHeight: 560
    title: "NoteBot Injector"
    color: "#08080C"

    property bool splashDone: false
    property real tick: 0
    property real mx: width / 2
    property real my: height / 2
    property real splashProgress: backend.downloadProgress >= 0 ? backend.downloadProgress : backend.initStep
    property bool readyForMain: splashDone && !backend.initializing

    readonly property color accentPurple: "#8B5CF6"
    readonly property color accentCyan: "#22D3EE"
    readonly property color accentPink: "#EC4899"
    readonly property color good: "#22C55E"
    readonly property color warn: "#F59E0B"
    readonly property color bad: "#EF4444"
    readonly property color textPrimary: "#F1F5F9"
    readonly property color textSecondary: "#64748B"
    readonly property color textMuted: "#475569"
    readonly property color surfaceCard: Qt.rgba(0.09, 0.09, 0.14, 0.92)
    readonly property color surfaceLine: Qt.rgba(1, 1, 1, 0.06)
    readonly property bool keyVisible: false

    function logViewNearBottom(view) {
        if (!view) return true
        return view.contentY >= Math.max(0, view.contentHeight - view.height - 8)
    }

    function stickLogViewToEnd(view, force) {
        if (!view) return
        if (backend.injectState === "injecting" || backend.initializing) force = true
        if (!force && !view.followTail) return
        Qt.callLater(function() {
            if (!view) return
            if (view.count > 0) {
                view.positionViewAtEnd()
            } else {
                view.contentY = 0
            }
        })
    }

    Timer {
        interval: 16
        running: true
        repeat: true
        onTriggered: tick += 0.016
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        onPositionChanged: function(m) {
            mx = m.x
            my = m.y
        }
    }

    Canvas {
        id: bgCanvas
        anchors.fill: parent
        z: -10
        property real t: tick
        onTChanged: requestPaint()
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            var ctx = getContext("2d")
            var w = width
            var h = height
            ctx.clearRect(0, 0, w, h)

            var bg = ctx.createLinearGradient(0, 0, w, h)
            bg.addColorStop(0, "#08080C")
            bg.addColorStop(0.5, "#0C0C14")
            bg.addColorStop(1, "#100C18")
            ctx.fillStyle = bg
            ctx.fillRect(0, 0, w, h)

            var bx1 = w * 0.3 + Math.sin(t * 0.3) * w * 0.15
            var by1 = h * 0.25 + Math.cos(t * 0.25) * h * 0.12
            var g1 = ctx.createRadialGradient(bx1, by1, 0, bx1, by1, 320)
            g1.addColorStop(0, Qt.rgba(0.545, 0.361, 0.965, 0.08))
            g1.addColorStop(1, "transparent")
            ctx.fillStyle = g1
            ctx.fillRect(0, 0, w, h)

            var bx2 = w * 0.7 + Math.cos(t * 0.35) * w * 0.12
            var by2 = h * 0.7 + Math.sin(t * 0.4) * h * 0.1
            var g2 = ctx.createRadialGradient(bx2, by2, 0, bx2, by2, 280)
            g2.addColorStop(0, Qt.rgba(0.133, 0.827, 0.933, 0.05))
            g2.addColorStop(1, "transparent")
            ctx.fillStyle = g2
            ctx.fillRect(0, 0, w, h)

            var bx3 = w * 0.5 + Math.sin(t * 0.2 + 2) * w * 0.2
            var by3 = h * 0.5 + Math.cos(t * 0.3 + 1) * h * 0.15
            var g3 = ctx.createRadialGradient(bx3, by3, 0, bx3, by3, 250)
            g3.addColorStop(0, Qt.rgba(0.925, 0.282, 0.6, 0.04))
            g3.addColorStop(1, "transparent")
            ctx.fillStyle = g3
            ctx.fillRect(0, 0, w, h)

            var mg = ctx.createRadialGradient(mx, my, 0, mx, my, 200)
            mg.addColorStop(0, Qt.rgba(0.545, 0.361, 0.965, 0.04))
            mg.addColorStop(1, "transparent")
            ctx.fillStyle = mg
            ctx.fillRect(0, 0, w, h)

            ctx.fillStyle = Qt.rgba(1, 1, 1, 0.015)
            for (var gy = 0; gy < h; gy += 40) {
                for (var gx = 0; gx < w; gx += 40) {
                    ctx.fillRect(gx, gy, 1, 1)
                }
            }
        }
    }

    component SideLabel: Text {
        color: textSecondary
        font.pixelSize: 10
        font.family: "Segoe UI"
        font.letterSpacing: 2
    }

    component FieldBox: Rectangle {
        radius: 8
        color: Qt.rgba(1, 1, 1, 0.04)
        border.color: surfaceLine
        border.width: 1
    }

    component SmallButton: Rectangle {
        id: smallBtn
        property string text: ""
        property color fill: Qt.rgba(1, 1, 1, 0.05)
        property color fillHover: Qt.rgba(1, 1, 1, 0.10)
        property color borderFill: Qt.rgba(1, 1, 1, 0.08)
        property color textFill: "#FFFFFF"
        property bool enabledState: true
        signal clicked()
        height: 32
        radius: 8
        opacity: enabledState ? 1 : 0.42
        color: enabledState && btnHover.hovered ? fillHover : fill
        border.color: borderFill
        border.width: 1
        Behavior on color { ColorAnimation { duration: 150 } }
        Text {
            anchors.centerIn: parent
            text: smallBtn.text
            color: smallBtn.textFill
            font.pixelSize: 12
            font.bold: true
            font.family: "Microsoft YaHei UI"
            elide: Text.ElideRight
        }
        HoverHandler { id: btnHover; enabled: smallBtn.enabledState }
        TapHandler {
            enabled: smallBtn.enabledState
            onTapped: smallBtn.clicked()
        }
    }
    Item {
        id: splash
        anchors.fill: parent
        z: 100
        visible: opacity > 0
        opacity: root.readyForMain ? 0 : 1
        Behavior on opacity { NumberAnimation { duration: 260; easing.type: Easing.InCubic } }

        Rectangle {
            id: logoBox
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: -230
            anchors.verticalCenterOffset: -56
            width: 90
            height: 90
            radius: 20
            color: "transparent"
            border.color: accentPurple
            border.width: 2.5
            scale: 0.985 + 0.008 * Math.sin(tick * 1.25)

            Text {
                anchors.centerIn: parent
                text: "N"
                color: accentPurple
                font.pixelSize: 48
                font.bold: true
                font.family: "Segoe UI"
            }
            Rectangle {
                anchors.centerIn: parent
                width: parent.width + 40
                height: parent.height + 40
                radius: 30
                color: "transparent"
                border.color: Qt.rgba(0.545, 0.361, 0.965, 0.09 + 0.025 * (0.5 + 0.5 * Math.sin(tick * 1.45)))
                border.width: 1
            }
        }

        Text {
            id: splashTitle
            anchors.horizontalCenter: logoBox.horizontalCenter
            anchors.top: logoBox.bottom
            anchors.topMargin: 20
            text: "NOTEBOT"
            color: textPrimary
            font.pixelSize: 28
            font.bold: true
            font.family: "Segoe UI"
            font.letterSpacing: 12
        }

        Text {
            anchors.horizontalCenter: logoBox.horizontalCenter
            anchors.top: splashTitle.bottom
            anchors.topMargin: 6
            text: "INJECTOR"
            color: textSecondary
            font.pixelSize: 13
            font.family: "Segoe UI"
            font.letterSpacing: 6
            opacity: 0.72
        }

        Rectangle {
            id: progressTrack
            anchors.horizontalCenter: logoBox.horizontalCenter
            anchors.top: splashTitle.bottom
            anchors.topMargin: 58
            width: 200
            height: 3
            radius: 1.5
            color: Qt.rgba(1, 1, 1, 0.06)

            Rectangle {
                width: Math.max(0, Math.min(parent.width, splashProgress / 100 * parent.width))
                height: parent.height
                radius: parent.radius
                gradient: Gradient {
                    GradientStop { position: 0; color: accentPurple }
                    GradientStop { position: 1; color: accentCyan }
                }
                Behavior on width { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
            }
        }

        Text {
            anchors.horizontalCenter: logoBox.horizontalCenter
            anchors.top: progressTrack.bottom
            anchors.topMargin: 12
            text: backend.downloadProgress >= 0
                  ? ("正在下载更新 " + Math.max(0, Math.min(100, backend.downloadProgress)) + "%")
                  : (backend.initStatus === "" ? "READY" : backend.initStatus)
            color: textSecondary
            font.pixelSize: 11
            font.family: "Microsoft YaHei UI"
            opacity: 0.72
        }

        Rectangle {
            id: splashLogPanel
            anchors.right: parent.right
            anchors.rightMargin: 32
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 32
            width: Math.min(460, Math.max(360, parent.width * 0.46))
            height: Math.max(190, Math.min(260, parent.height * 0.39))
            radius: 12
            color: Qt.rgba(0.07, 0.07, 0.11, 0.78)
            border.color: Qt.rgba(0.545, 0.361, 0.965, 0.18)
            border.width: 1

            Text {
                x: 14
                y: 10
                text: "LOG"
                color: Qt.rgba(1, 1, 1, 0.50)
                font.pixelSize: 10
                font.family: "Segoe UI"
                font.letterSpacing: 3
            }

            ListView {
                id: splashLogView
                property bool followTail: true
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                anchors.topMargin: 30
                anchors.bottomMargin: 14
                model: logModel
                clip: true
                spacing: 2
                boundsBehavior: Flickable.StopAtBounds

                onMovementStarted: {
                    if (!root.logViewNearBottom(splashLogView)) followTail = false
                }
                onMovementEnded: followTail = root.logViewNearBottom(splashLogView)
                onContentHeightChanged: root.stickLogViewToEnd(splashLogView, false)
                onCountChanged: root.stickLogViewToEnd(splashLogView, false)

                Connections {
                    target: logModel
                    function onLineAppended(line) {
                        root.stickLogViewToEnd(splashLogView, false)
                    }
                }

                delegate: Item {
                    width: splashLogView.width
                    height: splashLogText.implicitHeight + 2

                    Text {
                        id: splashLogText
                        width: parent.width
                        text: model.ts + "  " + model.text
                        color: {
                            var t = model.tag
                            if (t === "green") return good
                            if (t === "red") return bad
                            if (t === "orange") return warn
                            if (t === "cyan") return accentCyan
                            if (t === "accent") return accentPurple
                            return Qt.rgba(1, 1, 1, 0.70)
                        }
                        font.pixelSize: 11
                        font.family: "Consolas"
                        wrapMode: Text.WrapAnywhere
                        lineHeight: 1.15
                    }
                }
            }
        }

    }

    Timer {
        id: minimumSplash
        interval: 900
        running: true
        repeat: false
        onTriggered: splashDone = true
    }

    Connections {
        target: backend
        function onInitializationFinished() {
            backend.startScanning()
        }
    }

    Item {
        id: mainUI
        anchors.fill: parent
        opacity: root.readyForMain ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 360; easing.type: Easing.OutCubic } }

        property bool keyReady: keyInputRestored.text.trim().length > 0
        property bool injectFailureFlash: false
        property bool injectFailureCooldown: false
        property bool injectSuccessFlash: false
        property bool injectSuccessCooldown: false
        property real injectSuccessPulse: 0
        property string detailPanel: "log"
        property bool injectReady: backend.authSessionVerified && keyReady && backend.processCount > 0 && backend.selectedPid > 0 && !backend.injectCooldown && !injectFailureCooldown && !injectSuccessCooldown && backend.injectState !== "injecting"
        Connections {
            target: backend
            function onInjectStateChanged() {
                if (backend.injectState === "error") {
                    mainUI.injectFailureFlash = true
                    mainUI.injectFailureCooldown = false
                    mainUI.injectSuccessFlash = false
                    mainUI.injectSuccessCooldown = false
                    injectFailureFlashTimer.restart()
                    injectSuccessFlashTimer.stop()
                    injectSuccessCooldownTimer.stop()
                } else if (backend.injectState === "success") {
                    mainUI.injectFailureFlash = false
                    mainUI.injectFailureCooldown = false
                    mainUI.injectSuccessFlash = true
                    mainUI.injectSuccessCooldown = false
                    injectFailureFlashTimer.stop()
                    injectFailureCooldownTimer.stop()
                    injectSuccessFlashTimer.restart()
                } else {
                    mainUI.injectFailureFlash = false
                    mainUI.injectFailureCooldown = false
                    mainUI.injectSuccessFlash = false
                    mainUI.injectSuccessCooldown = false
                    injectFailureFlashTimer.stop()
                    injectFailureCooldownTimer.stop()
                    injectSuccessFlashTimer.stop()
                    injectSuccessCooldownTimer.stop()
                }
            }
        }

        Timer {
            id: injectFailureFlashTimer
            interval: 650
            repeat: false
            onTriggered: {
                mainUI.injectFailureFlash = false
                mainUI.injectFailureCooldown = true
                injectFailureCooldownTimer.restart()
            }
        }

        Timer {
            id: injectFailureCooldownTimer
            interval: 1400
            repeat: false
            onTriggered: mainUI.injectFailureCooldown = false
        }

        Timer {
            id: injectSuccessFlashTimer
            interval: 720
            repeat: false
            onTriggered: {
                mainUI.injectSuccessFlash = false
                mainUI.injectSuccessCooldown = true
                injectSuccessCooldownTimer.restart()
            }
        }

        Timer {
            id: injectSuccessCooldownTimer
            interval: 1250
            repeat: false
            onTriggered: mainUI.injectSuccessCooldown = false
        }

        SequentialAnimation on injectSuccessPulse {
            running: mainUI.injectSuccessCooldown
            loops: Animation.Infinite
            NumberAnimation { from: 0; to: 1; duration: 520; easing.type: Easing.InOutQuad }
            NumberAnimation { from: 1; to: 0; duration: 520; easing.type: Easing.InOutQuad }
        }

        Rectangle {
            id: restoredSidebar
            width: 220
            height: parent.height
            color: Qt.rgba(0.06, 0.06, 0.09, 0.95)
            z: 10

            Rectangle {
                anchors.right: parent.right
                width: 1
                height: parent.height
                color: Qt.rgba(1, 1, 1, 0.04)
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    spacing: 10
                    Rectangle {
                        width: 36
                        height: 36
                        radius: 10
                        color: accentPurple
                        Text {
                            anchors.centerIn: parent
                            text: "N"
                            color: "#FFFFFF"
                            font.pixelSize: 20
                            font.bold: true
                            font.family: "Segoe UI"
                        }
                    }
                    Column {
                        Layout.fillWidth: true
                        Text {
                            width: parent.width
                            text: "NoteBot"
                            color: textPrimary
                            font.pixelSize: 16
                            font.bold: true
                            font.family: "Segoe UI"
                            elide: Text.ElideRight
                        }
                        Text {
                            width: parent.width
                            text: "Injector v3"
                            color: textSecondary
                            font.pixelSize: 11
                            font.family: "Segoe UI"
                            elide: Text.ElideRight
                        }
                    }
                    Rectangle {
                        id: settingsButtonRestored
                        Layout.preferredWidth: 34
                        Layout.preferredHeight: 34
                        radius: 8
                        color: settingsTapRestored.pressed
                               ? Qt.rgba(0.545, 0.361, 0.965, 0.20)
                               : mainUI.detailPanel === "settings"
                                 ? Qt.rgba(0.545, 0.361, 0.965, 0.15)
                                 : settingsHoverRestored.hovered ? Qt.rgba(1, 1, 1, 0.08)
                                                                 : Qt.rgba(1, 1, 1, 0.035)
                        border.width: 1
                        border.color: mainUI.detailPanel === "settings"
                                      ? Qt.rgba(0.545, 0.361, 0.965, 0.48)
                                      : settingsHoverRestored.hovered ? Qt.rgba(1, 1, 1, 0.16)
                                                                      : Qt.rgba(1, 1, 1, 0.07)
                        scale: settingsTapRestored.pressed ? 0.94 : settingsHoverRestored.hovered ? 1.04 : 1
                        Behavior on color { ColorAnimation { duration: 130 } }
                        Behavior on border.color { ColorAnimation { duration: 130 } }
                        Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }

                        Text {
                            anchors.centerIn: parent
                            text: "⚙"
                            color: mainUI.detailPanel === "settings" ? "#F6F0FF" : textSecondary
                            font.pixelSize: 16
                            font.family: "Segoe UI Symbol"
                        }

                        HoverHandler { id: settingsHoverRestored }
                        TapHandler {
                            id: settingsTapRestored
                            onTapped: mainUI.detailPanel = "settings"
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Qt.rgba(1, 1, 1, 0.05)
                    Layout.topMargin: 8
                    Layout.bottomMargin: 16
                }

                Column {
                    Layout.fillWidth: true
                    spacing: 7

                    SideLabel { text: "VIEW" }

                    Row {
                        width: parent.width
                        height: 34
                        spacing: 7

                        Repeater {
                            model: [
                                { key: "log", label: "日志" },
                                { key: "models", label: "模型" }
                            ]

                            delegate: Rectangle {
                                width: (parent.width - parent.spacing) / 2
                                height: parent.height
                                radius: 7
                                property bool active: mainUI.detailPanel === modelData.key
                                color: active ? Qt.rgba(0.545, 0.361, 0.965, 0.18)
                                              : tabHover.hovered ? Qt.rgba(1, 1, 1, 0.055)
                                                                 : Qt.rgba(1, 1, 1, 0.025)
                                border.width: 1
                                border.color: active ? Qt.rgba(0.545, 0.361, 0.965, 0.46)
                                                     : Qt.rgba(1, 1, 1, 0.055)
                                Behavior on color { ColorAnimation { duration: 130 } }
                                Behavior on border.color { ColorAnimation { duration: 130 } }

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.label
                                    color: parent.active ? "#F6F0FF" : textSecondary
                                    font.pixelSize: 12
                                    font.bold: parent.active
                                    font.family: "Microsoft YaHei UI"
                                }

                                HoverHandler { id: tabHover }
                                TapHandler { onTapped: mainUI.detailPanel = modelData.key }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Qt.rgba(1, 1, 1, 0.05)
                    Layout.topMargin: 12
                    Layout.bottomMargin: 14
                }

                Column {
                    Layout.fillWidth: true
                    spacing: 10

                    Row {
                        spacing: 8
                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            anchors.verticalCenter: parent.verticalCenter
                            color: backend.processCount > 0 ? good : accentPurple
                            Rectangle {
                                anchors.centerIn: parent
                                width: 16
                                height: 16
                                radius: 8
                                color: parent.color
                                opacity: backend.processCount > 0 ? 0 : 0.3 * (0.5 + 0.5 * Math.sin(tick * 4))
                            }
                        }
                        Text {
                            text: backend.processCount > 0 ? (backend.processCount + " process found") : "Scanning..."
                            color: textSecondary
                            font.pixelSize: 12
                            font.family: "Segoe UI"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    Column {
                        width: parent.width
                        spacing: 4
                        SideLabel { text: "密钥" }
                        FieldBox {
                            width: parent.width
                            height: 38
                            border.color: keyInputRestored.activeFocus ? accentPurple : surfaceLine
                            TextInput {
                                id: keyInputRestored
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                anchors.leftMargin: 10
                                anchors.rightMargin: 6
                                anchors.topMargin: 8
                                anchors.bottomMargin: 8
                                color: textPrimary
                                font.pixelSize: 10
                                font.family: "Microsoft YaHei UI"
                                font.letterSpacing: 1
                                text: backend.licenseKey
                                echoMode: TextInput.Password
                                passwordCharacter: "●"
                                clip: true
                                selectByMouse: true
                                selectedTextColor: "#FFFFFF"
                                selectionColor: accentPurple
                                onEditingFinished: backend.licenseKey = text
                            }

                            Rectangle {
                                id: toggleKeyBtnRestored
                                visible: false
                                enabled: false
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.rightMargin: 8
                                width: 0
                                height: 24
                                radius: 6
                                color: keyToggleHoverRestored.hovered ? Qt.rgba(0.545, 0.361, 0.965, 0.16)
                                                                      : Qt.rgba(1, 1, 1, 0.05)
                                border.color: keyToggleHoverRestored.hovered ? Qt.rgba(0.545, 0.361, 0.965, 0.34)
                                                                             : Qt.rgba(1, 1, 1, 0.08)
                                border.width: 1
                                Text {
                                    anchors.centerIn: parent
                                    text: ""
                                    color: Qt.rgba(1, 1, 1, 0)
                                    font.pixelSize: 11
                                    font.bold: true
                                    font.family: "Microsoft YaHei UI"
                                }
                                HoverHandler { id: keyToggleHoverRestored }
                                TapHandler { enabled: false }
                            }
                        }
                    }

                    Rectangle {
                        id: authCheckBtnRestored
                        width: parent.width
                        height: 36
                        radius: 6
                        property bool ready: keyInputRestored.text.trim().length > 0
                        enabled: true
                        color: authCheckBtnRestored.ready
                               ? (authTapRestored.pressed ? "#181421" : authHoverRestored.hovered ? "#211A31" : "#191522")
                               : "#181820"
                        border.width: 1
                        border.color: authCheckBtnRestored.ready
                                      ? (authHoverRestored.hovered ? "#8B5CF6" : "#6842D8")
                                      : "#6F4CD8"
                        opacity: 1
                        scale: authTapRestored.pressed && authCheckBtnRestored.ready ? 0.988 : 1
                        Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }
                        Behavior on color { ColorAnimation { duration: 110 } }
                        Behavior on border.color { ColorAnimation { duration: 110 } }

                        Text {
                            anchors.fill: parent
                            text: "联网验证 / 激活"
                            color: "#F6F0FF"
                            font.pixelSize: 13
                            font.bold: true
                            font.family: "Microsoft YaHei UI"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        HoverHandler { id: authHoverRestored; enabled: authCheckBtnRestored.ready }
                        TapHandler {
                            id: authTapRestored
                            enabled: authCheckBtnRestored.ready
                            onTapped: {
                                backend.licenseKey = keyInputRestored.text
                                backend.verifyLicense()
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Qt.rgba(1, 1, 1, 0.05)
                    Layout.topMargin: 16
                    Layout.bottomMargin: 16
                }

                Rectangle {
                    id: restoredStatusCard
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    radius: 6
                    property string statusText: {
                        if (backend.licenseStatus && backend.licenseStatus.length > 0) {
                            if (backend.licenseStatus.indexOf("待联网激活") >= 0) {
                                return "待激活"
                            }
                            var tierText = ""
                            if (backend.licenseTier === "Dev") tierText = "开发版"
                            else if (backend.licenseTier === "Premium") tierText = "高级版"
                            else if (backend.licenseTier === "Trial") tierText = "试用版"
                            else if (backend.licenseTier !== "None" && backend.licenseTier !== "") tierText = backend.licenseTier
                            return backend.licenseStatus + (tierText !== "" ? " / " + tierText : "")
                        }
                        return "请输入密钥后进行联网验证"
                    }
                    color: {
                        if (backend.isActivated) return "#101915"
                        if (backend.licenseStatus.indexOf("离线") >= 0) return "#1E1B12"
                        if (backend.licenseStatus.indexOf("待联网激活") >= 0) return "#181820"
                        if (backend.licenseStatus.indexOf("顶下线") >= 0) return "#211418"
                        return "#181820"
                    }
                    border.color: {
                        if (backend.isActivated) return "#22B573"
                        if (backend.licenseStatus.indexOf("离线") >= 0) return "#A87A24"
                        if (backend.licenseStatus.indexOf("待联网激活") >= 0) return Qt.rgba(1, 1, 1, 0.10)
                        if (backend.licenseStatus.indexOf("顶下线") >= 0) return "#B84250"
                        return Qt.rgba(1, 1, 1, 0.10)
                    }
                    border.width: 1

                    Text {
                        anchors.fill: parent
                        text: restoredStatusCard.statusText
                        color: {
                            if (backend.isActivated) return "#A7F3C6"
                            if (backend.licenseStatus.indexOf("离线") >= 0) return "#E8BE65"
                            if (backend.licenseStatus.indexOf("待联网激活") >= 0) return "#F4F6FB"
                            if (backend.licenseStatus.indexOf("顶下线") >= 0) return "#EF8E99"
                            return "#F4F6FB"
                        }
                        font.pixelSize: 12
                        font.family: "Microsoft YaHei UI"
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: backend.injectProgress >= 0 ? 52 : 40
                    radius: 8
                    color: Qt.rgba(1, 1, 1, 0.03)
                    border.color: Qt.rgba(1, 1, 1, 0.07)
                    border.width: 1

                    Column {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 6

                        Text {
                            width: parent.width
                            text: backend.injectStageText && backend.injectStageText.length > 0
                                  ? backend.injectStageText
                                  : "待命"
                            color: backend.injectState === "error"
                                   ? bad
                                   : backend.injectState === "success"
                                     ? good
                                     : textPrimary
                            font.pixelSize: 11
                            font.family: "Microsoft YaHei UI"
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        Rectangle {
                            visible: backend.injectProgress >= 0
                            width: parent.width
                            height: 4
                            radius: 2
                            color: Qt.rgba(1, 1, 1, 0.06)

                            Rectangle {
                                width: Math.max(0, Math.min(parent.width, backend.injectProgress / 100 * parent.width))
                                height: parent.height
                                radius: parent.radius
                                gradient: Gradient {
                                    GradientStop { position: 0; color: accentPurple }
                                    GradientStop { position: 1; color: accentCyan }
                                }
                                Behavior on width { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                Rectangle {
                    id: modelReplacementStrip
                    visible: true
                    Layout.fillWidth: true
                    Layout.preferredHeight: visible ? 42 : 0
                    radius: 8
                    color: modelReplacementTapRestored.pressed
                           ? (backend.modelModificationEnabled ? Qt.rgba(0.133, 0.827, 0.933, 0.13) : Qt.rgba(1, 1, 1, 0.085))
                           : backend.modelModificationEnabled
                             ? (modelReplacementHoverRestored.hovered ? Qt.rgba(0.133, 0.827, 0.933, 0.10) : Qt.rgba(0.133, 0.827, 0.933, 0.055))
                             : (modelReplacementHoverRestored.hovered ? Qt.rgba(1, 1, 1, 0.062) : Qt.rgba(1, 1, 1, 0.025))
                    border.color: backend.modelModificationEnabled
                                  ? (modelReplacementHoverRestored.hovered ? Qt.rgba(0.133, 0.827, 0.933, 0.58) : Qt.rgba(0.133, 0.827, 0.933, 0.34))
                                  : (modelReplacementHoverRestored.hovered ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(1, 1, 1, 0.07))
                    border.width: modelReplacementHoverRestored.hovered ? 1.25 : 1
                    scale: modelReplacementTapRestored.pressed ? 0.982 : modelReplacementHoverRestored.hovered ? 1.01 : 1
                    transformOrigin: Item.Center

                    Behavior on color { ColorAnimation { duration: 130 } }
                    Behavior on scale { NumberAnimation { duration: 105; easing.type: Easing.OutCubic } }

                    Rectangle {
                        width: modelReplacementTapRestored.pressed ? 5 : modelReplacementHoverRestored.hovered ? 4 : 3
                        height: parent.height - 14
                        radius: 2
                        anchors.left: parent.left
                        anchors.leftMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        color: backend.modelModificationEnabled
                               ? accentCyan
                               : (modelReplacementHoverRestored.hovered ? textSecondary : textMuted)
                        opacity: backend.modelModificationEnabled ? 0.92 : 0.55
                        Behavior on width { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                        Behavior on color { ColorAnimation { duration: 130 } }
                        Behavior on opacity { NumberAnimation { duration: 130 } }
                    }

                    HoverHandler {
                        id: modelReplacementHoverRestored
                    }

                    TapHandler {
                        id: modelReplacementTapRestored
                        onTapped: backend.modelModificationEnabled = !backend.modelModificationEnabled
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 8
                        anchors.topMargin: 8
                        anchors.bottomMargin: 8
                        spacing: 8

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1

                            Text {
                                Layout.fillWidth: true
                                text: "皮肤替换"
                                color: backend.modelModificationEnabled
                                       ? (modelReplacementHoverRestored.hovered ? "#CFFAFE" : textPrimary)
                                       : (modelReplacementHoverRestored.hovered ? textPrimary : textSecondary)
                                font.pixelSize: 11
                                font.bold: true
                                font.family: "Microsoft YaHei UI"
                                elide: Text.ElideRight
                                Behavior on color { ColorAnimation { duration: 130 } }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: backend.modelModificationEnabled
                                      ? "开：等待新进程"
                                      : "关：不改启动"
                                color: backend.modelModificationEnabled
                                       ? (modelReplacementHoverRestored.hovered ? accentCyan : textSecondary)
                                       : (modelReplacementHoverRestored.hovered ? textSecondary : textMuted)
                                font.pixelSize: 10
                                font.family: "Microsoft YaHei UI"
                                elide: Text.ElideRight
                                Behavior on color { ColorAnimation { duration: 130 } }
                            }
                        }

                        Rectangle {
                            id: modelReplacementStateBadge
                            Layout.preferredWidth: 72
                            Layout.preferredHeight: 26
                            radius: 7
                            color: backend.modelReplacementStatus === "失败"
                                   ? Qt.rgba(0.937, 0.267, 0.267, 0.14)
                                   : backend.modelReplacementRunning
                                     ? (modelReplacementHoverRestored.hovered ? Qt.rgba(0.133, 0.827, 0.933, 0.22) : Qt.rgba(0.133, 0.827, 0.933, 0.15))
                                     : (modelReplacementHoverRestored.hovered ? Qt.rgba(1, 1, 1, 0.085) : Qt.rgba(1, 1, 1, 0.055))
                            border.color: backend.modelReplacementStatus === "失败"
                                          ? Qt.rgba(0.937, 0.267, 0.267, 0.34)
                                          : backend.modelReplacementRunning
                                            ? (modelReplacementHoverRestored.hovered ? Qt.rgba(0.133, 0.827, 0.933, 0.52) : Qt.rgba(0.133, 0.827, 0.933, 0.36))
                                            : (modelReplacementHoverRestored.hovered ? Qt.rgba(1, 1, 1, 0.18) : Qt.rgba(1, 1, 1, 0.10))
                            border.width: 1
                            scale: modelReplacementTapRestored.pressed ? 0.96 : 1
                            Behavior on color { ColorAnimation { duration: 130 } }
                            Behavior on scale { NumberAnimation { duration: 105; easing.type: Easing.OutCubic } }

                            Text {
                                anchors.fill: parent
                                text: backend.modelReplacementStatus
                                color: backend.modelReplacementStatus === "失败"
                                       ? "#FCA5A5"
                                       : backend.modelReplacementRunning
                                         ? accentCyan
                                         : textSecondary
                                font.pixelSize: 10
                                font.bold: true
                                font.family: "Microsoft YaHei UI"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }
                        }
                    }
                }

                Rectangle {
                    id: injectBtnRestored
                    Layout.fillWidth: true
                    height: 48
                    radius: 10
                    property string st: backend.injectState
                    enabled: mainUI.injectReady
                    opacity: 1
                    gradient: Gradient {
                        GradientStop {
                            position: 0
                            color: {
                                if (mainUI.injectFailureFlash && injectBtnRestored.st === "error") return "#DC2626"
                                if (mainUI.injectSuccessFlash && injectBtnRestored.st === "success") return "#16A34A"
                                if (mainUI.injectSuccessCooldown) return "#355A5A"
                                if (!mainUI.injectReady) return "#452A68"
                                if (injectBtnRestored.st === "injecting") return "#6D28D9"
                                return injectHoverRestored.hovered ? "#7C3AED" : "#6D28D9"
                            }
                        }
                        GradientStop {
                            position: 1
                            color: {
                                if (mainUI.injectFailureFlash && injectBtnRestored.st === "error") return "#B91C1C"
                                if (mainUI.injectSuccessFlash && injectBtnRestored.st === "success") return "#15803D"
                                if (mainUI.injectSuccessCooldown) return "#284B6D"
                                if (!mainUI.injectReady) return "#2C1D49"
                                if (injectBtnRestored.st === "injecting") return "#5B21B6"
                                return injectHoverRestored.hovered ? "#6D28D9" : "#5B21B6"
                            }
                        }
                    }
                    scale: injectTapRestored.pressed ? 0.96 : 1
                    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }

                    Text {
                        anchors.centerIn: parent
                        text: {
                            if (backend.injectCooldown || mainUI.injectFailureCooldown || mainUI.injectSuccessCooldown) return "WAIT..."
                            if (injectBtnRestored.st === "injecting") return "INJECTING..."
                            if (mainUI.injectSuccessFlash && injectBtnRestored.st === "success") return "SUCCESS"
                            if (injectBtnRestored.st === "error" && mainUI.injectFailureFlash) return "FAILED"
                            return "INJECT"
                        }
                        color: (backend.injectCooldown || mainUI.injectFailureCooldown || mainUI.injectSuccessCooldown || mainUI.injectSuccessFlash)
                               ? "#FFFFFF"
                               : (mainUI.injectReady ? "#FFFFFF" : Qt.rgba(1, 1, 1, 0.52))
                        font.pixelSize: 14
                        font.bold: true
                        font.family: "Segoe UI"
                        font.letterSpacing: 2
                    }

                    Rectangle {
                        anchors.fill: parent
                        radius: parent.radius
                        z: -1
                        visible: mainUI.injectSuccessCooldown
                        color: "#FFFFFF"
                        opacity: 0.04 + mainUI.injectSuccessPulse * 0.08
                    }
                    HoverHandler { id: injectHoverRestored; enabled: injectBtnRestored.enabled }
                    TapHandler {
                        id: injectTapRestored
                        enabled: injectBtnRestored.enabled
                        onTapped: backend.doInject()
                    }
                }
            }
        }

        Item {
            id: restoredContentArea
            anchors.left: restoredSidebar.right
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            z: 10

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 12

                Item {
                    visible: mainUI.detailPanel !== "settings"
                    Layout.fillWidth: true
                    Layout.fillHeight: false
                    Layout.preferredHeight: 150
                    Layout.minimumHeight: 140
                    Layout.maximumHeight: 170

                    Text {
                        x: 4
                        y: 0
                        text: "PROCESSES"
                        color: textSecondary
                        font.pixelSize: 11
                        font.family: "Segoe UI"
                        font.letterSpacing: 3
                    }

                    Text {
                        anchors.right: parent.right
                        anchors.top: parent.top
                        text: backend.processCount + " 个"
                        color: textSecondary
                        font.pixelSize: 10
                        font.family: "Segoe UI"
                    }

                    ListView {
                        id: procListRestored
                        anchors.fill: parent
                        anchors.topMargin: 28
                        model: processModel
                        clip: true
                        spacing: 6
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                        delegate: Rectangle {
                            width: procListRestored.width
                            height: 56
                            radius: 10
                            color: model.isSelected ? Qt.rgba(0.545, 0.361, 0.965, 0.12)
                                                    : cardMouseRestored.hovered ? Qt.rgba(1, 1, 1, 0.04)
                                                                                : Qt.rgba(1, 1, 1, 0.02)
                            border.color: model.isSelected ? Qt.rgba(0.545, 0.361, 0.965, 0.4) : "transparent"
                            border.width: model.isSelected ? 1 : 0
                            Behavior on color { ColorAnimation { duration: 180 } }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 14
                                anchors.rightMargin: 14
                                spacing: 12

                                Rectangle {
                                    width: 8
                                    height: 8
                                    radius: 4
                                    Layout.alignment: Qt.AlignVCenter
                                    color: model.hasWindow ? good : warn
                                }

                                Column {
                                    Layout.fillWidth: true
                                    Layout.alignment: Qt.AlignVCenter
                                    spacing: 2
                                    Text {
                                        text: model.exe + "  / PID " + model.pid
                                        color: model.isSelected ? accentPurple : textPrimary
                                        font.pixelSize: 13
                                        font.bold: true
                                        font.family: "Consolas"
                                        elide: Text.ElideRight
                                        width: parent.width
                                    }
                                    Text {
                                        text: model.winTitle && model.winTitle.length > 0 ? model.winTitle : model.path
                                        color: textSecondary
                                        font.pixelSize: 11
                                        font.family: "Segoe UI"
                                        elide: Text.ElideRight
                                        width: parent.width
                                    }
                                }

                                Row {
                                    spacing: 6
                                    Layout.alignment: Qt.AlignVCenter
                                    Rectangle {
                                        visible: model.hasWindow
                                        width: 56
                                        height: 26
                                        radius: 6
                                        color: fgHoverRestored.hovered ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(1, 1, 1, 0.03)
                                        border.color: fgHoverRestored.hovered ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(1, 1, 1, 0.06)
                                        Text {
                                            anchors.centerIn: parent
                                            text: "Focus"
                                            color: textSecondary
                                            font.pixelSize: 11
                                            font.family: "Segoe UI"
                                        }
                                        HoverHandler { id: fgHoverRestored }
                                        TapHandler { onTapped: backend.bringToFront(model.pid) }
                                    }

                                    Rectangle {
                                        width: 56
                                        height: 26
                                        radius: 6
                                        color: model.isSelected ? Qt.rgba(0.545, 0.361, 0.965, 0.2)
                                                                : selHoverRestored.hovered ? Qt.rgba(1, 1, 1, 0.08)
                                                                                           : Qt.rgba(1, 1, 1, 0.03)
                                        border.color: model.isSelected ? accentPurple
                                                                       : selHoverRestored.hovered ? Qt.rgba(1, 1, 1, 0.12)
                                                                                                  : Qt.rgba(1, 1, 1, 0.06)
                                        Text {
                                            anchors.centerIn: parent
                                            text: model.isSelected ? "OK" : "Select"
                                            color: model.isSelected ? accentPurple : textSecondary
                                            font.pixelSize: 11
                                            font.family: "Segoe UI"
                                        }
                                        HoverHandler { id: selHoverRestored }
                                        TapHandler { onTapped: backend.selectProcess(model.pid) }
                                    }
                                }
                            }

                            HoverHandler { id: cardMouseRestored }
                            TapHandler { onTapped: backend.selectProcess(model.pid) }
                        }
                    }
                }

                Item {
                    visible: mainUI.detailPanel !== "settings"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }

                Rectangle {
                    visible: mainUI.detailPanel !== "settings"
                    Layout.fillWidth: true
                    Layout.fillHeight: false
                    Layout.preferredHeight: Math.max(320, Math.floor(restoredContentArea.height * 0.5))
                    Layout.minimumHeight: 320
                    radius: 10
                    color: Qt.rgba(1, 1, 1, 0.018)
                    border.color: Qt.rgba(1, 1, 1, 0.035)
                    border.width: 1

                    Text {
                        x: 12
                        y: 8
                        text: mainUI.detailPanel === "models"
                              ? "MODELS"
                              : "LOG"
                        color: textSecondary
                        font.pixelSize: 10
                        font.family: "Segoe UI"
                        font.letterSpacing: 2
                    }

                    ListView {
                        id: logViewRestored
                        property bool followTail: true
                        anchors.fill: parent
                        anchors.margins: 10
                        anchors.topMargin: 26
                        model: logModel
                        clip: true
                        visible: mainUI.detailPanel === "log"
                        spacing: 0
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                        onMovementStarted: {
                            if (!root.logViewNearBottom(logViewRestored)) followTail = false
                        }
                        onMovementEnded: followTail = root.logViewNearBottom(logViewRestored)
                        onContentHeightChanged: root.stickLogViewToEnd(logViewRestored, false)
                        onCountChanged: root.stickLogViewToEnd(logViewRestored, false)

                        Connections {
                            target: logModel
                            function onLineAppended(line) {
                                root.stickLogViewToEnd(logViewRestored, false)
                            }
                        }

                        delegate: Item {
                            width: logViewRestored.width
                            height: restoredLogText.implicitHeight + 2

                            Text {
                                id: restoredLogText
                                width: parent.width
                                text: model.ts + "  " + model.text
                                color: {
                                    var t = model.tag
                                    if (t === "green") return good
                                    if (t === "red") return bad
                                    if (t === "orange") return warn
                                    if (t === "cyan") return accentCyan
                                    if (t === "accent") return accentPurple
                                    return Qt.rgba(1, 1, 1, 0.70)
                                }
                                font.pixelSize: 11
                                font.family: "Consolas"
                                wrapMode: Text.WrapAnywhere
                                lineHeight: 1.15
                            }
                        }
                    }

                    Item {
                        id: modelPanelRestored
                        anchors.fill: parent
                        anchors.margins: 10
                        anchors.topMargin: 26
                        visible: mainUI.detailPanel === "models"
                        clip: true

                        GridView {
                            id: modelGridRestored
                            anchors.fill: parent
                            clip: true
                            visible: modelCatalogModel.count > 0
                            model: modelCatalogModel
                            property int columns: Math.max(3, Math.floor(width / 210))
                            cellWidth: Math.floor(width / columns)
                            cellHeight: Math.max(266, Math.min(296, height - 6))
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                            delegate: Item {
                                width: modelGridRestored.cellWidth
                                height: modelGridRestored.cellHeight

                                Rectangle {
                                    anchors.fill: parent
                                    anchors.margins: 6
                                    radius: 8
                                    color: active ? Qt.rgba(0.180, 0.760, 0.500, 0.14)
                                                  : modelCardHover.hovered ? Qt.rgba(1, 1, 1, 0.046)
                                                                           : Qt.rgba(1, 1, 1, 0.026)
                                    border.width: 1
                                    border.color: active ? Qt.rgba(0.180, 0.760, 0.500, 0.52)
                                                         : modelCardHover.hovered ? Qt.rgba(0.545, 0.361, 0.965, 0.38)
                                                                                  : Qt.rgba(1, 1, 1, 0.055)
                                    Behavior on color { ColorAnimation { duration: 140 } }
                                    Behavior on border.color { ColorAnimation { duration: 140 } }

                                    Rectangle {
                                        id: modelShot
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: parent.top
                                        anchors.margins: 8
                                        height: Math.max(194, parent.height - 88)
                                        radius: 7
                                        color: "#101216"
                                        clip: true

                                        ModelPreview {
                                            id: previewItemRestored
                                            anchors.fill: parent
                                            visible: owned && modelFile.length > 0 && textureFile.length > 0
                                            modelSource: modelFile
                                            textureSource: textureFile
                                            autoRotate: mainUI.detailPanel === "models"
                                            yaw: startYaw
                                            Component.onCompleted: captureDefaultView()
                                        }

                                        Image {
                                            anchors.fill: parent
                                            visible: !owned && coverFile.length > 0
                                            source: coverFile
                                            fillMode: Image.PreserveAspectFit
                                            smooth: true
                                        }

                                        Column {
                                            anchors.left: parent.left
                                            anchors.bottom: parent.bottom
                                            anchors.margins: 12
                                            spacing: 2

                                            Text {
                                                text: "3D 预览"
                                                color: Qt.rgba(1, 1, 1, 0.70)
                                                font.pixelSize: 10
                                                font.family: "Microsoft YaHei UI"
                                            }

                                            Text {
                                                text: "拖拽旋转 / 滚轮缩放"
                                                color: Qt.rgba(1, 1, 1, 0.44)
                                                font.pixelSize: 10
                                                font.family: "Microsoft YaHei UI"
                                            }
                                        }

                                        Rectangle {
                                            anchors.left: parent.left
                                            anchors.right: parent.right
                                            anchors.bottom: parent.bottom
                                            height: 42
                                            gradient: Gradient {
                                                GradientStop { position: 0; color: Qt.rgba(0, 0, 0, 0) }
                                                GradientStop { position: 1; color: Qt.rgba(0, 0, 0, 0.58) }
                                            }
                                        }
                                    }

                                    Item {
                                        anchors.left: parent.left
                                        anchors.right: parent.right
                                        anchors.top: modelShot.bottom
                                        anchors.bottom: parent.bottom
                                        anchors.leftMargin: 10
                                        anchors.rightMargin: 10
                                        anchors.topMargin: 8

                                        Column {
                                            anchors.left: parent.left
                                            anchors.right: stateBadge.left
                                            anchors.rightMargin: 10
                                            anchors.top: parent.top
                                            spacing: 2
                                            Text {
                                                width: parent.width
                                                text: name
                                                color: textPrimary
                                                font.pixelSize: 13
                                                font.bold: true
                                                font.family: "Microsoft YaHei UI"
                                                elide: Text.ElideRight
                                            }
                                            Text {
                                                width: parent.width
                                                text: subtitle
                                                visible: text.length > 0
                                                color: Qt.rgba(1, 1, 1, 0.48)
                                                font.pixelSize: 10
                                                font.family: "Microsoft YaHei UI"
                                                elide: Text.ElideRight
                                            }
                                        }

                                        Row {
                                            anchors.left: parent.left
                                            anchors.right: stateBadge.left
                                            anchors.rightMargin: 10
                                            anchors.bottom: parent.bottom
                                            anchors.bottomMargin: 2
                                            spacing: 7
                                            Text {
                                                text: footerLeft
                                                color: textMuted
                                                font.pixelSize: 10
                                                font.family: "Consolas"
                                            }
                                            Text {
                                                text: footerRight
                                                color: textMuted
                                                font.pixelSize: 10
                                                font.family: "Consolas"
                                            }
                                        }

                                        Rectangle {
                                            id: stateBadge
                                            anchors.right: parent.right
                                            anchors.top: parent.top
                                            width: 60
                                            height: 22
                                            radius: 5
                                            color: stateCode === "active"
                                                   ? Qt.rgba(0.180, 0.760, 0.500, 0.13)
                                                   : stateCode === "owned"
                                                     ? Qt.rgba(0.250, 0.600, 0.980, 0.12)
                                                     : Qt.rgba(0.133, 0.827, 0.933, 0.10)
                                            border.width: 1
                                            border.color: stateCode === "active"
                                                          ? Qt.rgba(0.180, 0.760, 0.500, 0.42)
                                                          : stateCode === "owned"
                                                            ? Qt.rgba(0.250, 0.600, 0.980, 0.34)
                                                            : Qt.rgba(0.133, 0.827, 0.933, 0.28)
                                            Text {
                                                anchors.centerIn: parent
                                                text: stateLabel
                                                color: stateCode === "active" ? "#7CFFB5"
                                                      : stateCode === "owned" ? "#7CC7FF"
                                                      : accentCyan
                                                font.pixelSize: 10
                                                font.family: "Microsoft YaHei UI"
                                            }
                                        }
                                    }
                                    HoverHandler { id: modelCardHover }
                                    TapHandler {
                                        onTapped: {
                                            if (owned) {
                                                backend.activateModel(modelId)
                                            }
                                        }
                                    }
                            }
                        }

                        Rectangle {
                            anchors.fill: parent
                            visible: modelCatalogModel.count === 0
                            radius: 10
                            color: Qt.rgba(1, 1, 1, 0.022)
                            border.width: 1
                            border.color: Qt.rgba(1, 1, 1, 0.055)

                            Column {
                                anchors.centerIn: parent
                                spacing: 8

                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "当前密钥暂无可用模型"
                                    color: textPrimary
                                    font.pixelSize: 15
                                    font.family: "Microsoft YaHei UI"
                                    font.bold: true
                                }

                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: "完成密钥检查后会按云端授权自动拉取模型列表"
                                    color: textSecondary
                                    font.pixelSize: 11
                                    font.family: "Microsoft YaHei UI"
                                }
                                }
                            }
                        }
                    }

        }

            Item {
                id: settingsPageRestored
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: mainUI.detailPanel === "settings"

                property int settingsTab: 0

                Column {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.leftMargin: 12
                    anchors.topMargin: 10
                    width: settingsPageRestored.settingsTab === 1
                           ? Math.min(620, Math.max(0, parent.width - 24))
                           : Math.min(440, Math.max(0, parent.width - 24))
                    spacing: 14

                    Rectangle {
                        width: parent.width
                        height: 36
                        radius: 8
                        color: Qt.rgba(1, 1, 1, 0.026)
                        border.width: 1
                        border.color: Qt.rgba(1, 1, 1, 0.075)

                        Row {
                            anchors.fill: parent
                            anchors.margins: 4
                            spacing: 4

                            Rectangle {
                                width: (parent.width - parent.spacing) / 2
                                height: parent.height
                                radius: 6
                                property bool selected: settingsPageRestored.settingsTab === 0
                                color: selected ? Qt.rgba(0.133, 0.827, 0.933, 0.14) : skinTabHover.hovered ? Qt.rgba(1, 1, 1, 0.045) : "transparent"
                                border.width: selected ? 1 : 0
                                border.color: Qt.rgba(0.133, 0.827, 0.933, 0.42)

                                Text {
                                    anchors.centerIn: parent
                                    text: "皮肤替换"
                                    color: parent.selected ? accentCyan : textSecondary
                                    font.pixelSize: 12
                                    font.bold: parent.selected
                                    font.family: "Microsoft YaHei UI"
                                }

                                HoverHandler { id: skinTabHover }
                                TapHandler { onTapped: settingsPageRestored.settingsTab = 0 }
                            }

                            Rectangle {
                                width: (parent.width - parent.spacing) / 2
                                height: parent.height
                                radius: 6
                                property bool selected: settingsPageRestored.settingsTab === 1
                                color: selected ? Qt.rgba(0.545, 0.361, 0.965, 0.14) : hotkeyTabHover.hovered ? Qt.rgba(1, 1, 1, 0.045) : "transparent"
                                border.width: selected ? 1 : 0
                                border.color: Qt.rgba(0.545, 0.361, 0.965, 0.42)

                                Text {
                                    anchors.centerIn: parent
                                    text: "快捷键"
                                    color: parent.selected ? "#DDD6FE" : textSecondary
                                    font.pixelSize: 12
                                    font.bold: parent.selected
                                    font.family: "Microsoft YaHei UI"
                                }

                                HoverHandler { id: hotkeyTabHover }
                                TapHandler { onTapped: settingsPageRestored.settingsTab = 1 }
                            }
                        }
                    }

                    Item {
                        width: parent.width
                        height: settingsPageRestored.settingsTab === 0 ? skinSettingsContent.implicitHeight : hotkeySettingsContent.implicitHeight

                        Column {
                            id: skinSettingsContent
                            visible: settingsPageRestored.settingsTab === 0
                            width: parent.width
                            spacing: 10

                            Column {
                                width: parent.width
                                spacing: 3

                                Text {
                                    width: parent.width
                                    text: "启动皮肤替换"
                                    color: textPrimary
                                    font.pixelSize: 16
                                    font.bold: true
                                    font.family: "Microsoft YaHei UI"
                                    elide: Text.ElideRight
                                }

                                Text {
                                    width: parent.width
                                    text: backend.classicModeEnabled ? "经典方块人优先" : "服务器立体模型"
                                    color: backend.classicModeEnabled ? accentCyan : textSecondary
                                    font.pixelSize: 11
                                    font.family: "Microsoft YaHei UI"
                                    elide: Text.ElideRight
                                }
                            }

                            Rectangle {
                                width: parent.width
                                height: 44
                                radius: 8
                                color: Qt.rgba(1, 1, 1, 0.026)
                                border.width: 1
                                border.color: Qt.rgba(1, 1, 1, 0.075)

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 10
                                    spacing: 10

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 1

                                        Text {
                                            Layout.fillWidth: true
                                            text: "皮肤 PNG"
                                            color: textSecondary
                                            font.pixelSize: 12
                                            font.bold: true
                                            font.family: "Microsoft YaHei UI"
                                            elide: Text.ElideRight
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            text: backend.skinPngPath === "" ? "未选择" : backend.skinPngPath.split('/').pop().split('\\').pop()
                                            color: backend.skinPngPath === "" ? textMuted : accentCyan
                                            font.pixelSize: 10
                                            font.family: "Microsoft YaHei UI"
                                            elide: Text.ElideRight
                                        }
                                    }

                                    Text {
                                        visible: backend.skinPngWidth > 0
                                        Layout.preferredWidth: 56
                                        text: backend.skinPngWidth + "×" + backend.skinPngHeight
                                        color: textSecondary
                                        font.pixelSize: 10
                                        font.family: "Segoe UI"
                                        horizontalAlignment: Text.AlignRight
                                    }

                                    Rectangle {
                                        Layout.preferredWidth: 56
                                        Layout.preferredHeight: 26
                                        radius: 6
                                        color: skinPickerHover.hovered ? Qt.rgba(0.133, 0.827, 0.933, 0.16) : Qt.rgba(1, 1, 1, 0.05)
                                        border.width: 1
                                        border.color: skinPickerHover.hovered ? Qt.rgba(0.133, 0.827, 0.933, 0.48) : Qt.rgba(1, 1, 1, 0.12)

                                        Text {
                                            anchors.centerIn: parent
                                            text: "选择"
                                            color: skinPickerHover.hovered ? accentCyan : textSecondary
                                            font.pixelSize: 11
                                            font.bold: true
                                            font.family: "Microsoft YaHei UI"
                                        }

                                        HoverHandler { id: skinPickerHover }
                                        TapHandler {
                                            id: skinPickerTap
                                            onTapped: skinFileDialog.open()
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                id: classicModeToggle
                                width: parent.width
                                height: 44
                                radius: 8
                                color: classicTap.pressed
                                       ? (backend.classicModeEnabled ? Qt.rgba(0.133, 0.827, 0.933, 0.13) : Qt.rgba(1, 1, 1, 0.085))
                                       : backend.classicModeEnabled
                                         ? (classicHover.hovered ? Qt.rgba(0.133, 0.827, 0.933, 0.10) : Qt.rgba(0.133, 0.827, 0.933, 0.055))
                                         : (classicHover.hovered ? Qt.rgba(1, 1, 1, 0.062) : Qt.rgba(1, 1, 1, 0.025))
                                border.width: 1
                                border.color: backend.classicModeEnabled
                                              ? (classicHover.hovered ? Qt.rgba(0.133, 0.827, 0.933, 0.58) : Qt.rgba(0.133, 0.827, 0.933, 0.34))
                                              : (classicHover.hovered ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(1, 1, 1, 0.07))

                                HoverHandler { id: classicHover }
                                TapHandler {
                                    id: classicTap
                                    onTapped: backend.classicModeEnabled = !backend.classicModeEnabled
                                }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 10
                                    spacing: 10

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 1

                                        Text {
                                            Layout.fillWidth: true
                                            text: "经典方块人覆盖"
                                            color: backend.classicModeEnabled ? textPrimary : textSecondary
                                            font.pixelSize: 13
                                            font.bold: true
                                            font.family: "Microsoft YaHei UI"
                                            elide: Text.ElideRight
                                        }

                                        Text {
                                            Layout.fillWidth: true
                                            text: backend.classicModeEnabled ? "On · 方块人皮肤" : "Off · 服务器模型"
                                            color: backend.classicModeEnabled ? accentCyan : textMuted
                                            font.pixelSize: 10
                                            font.family: "Microsoft YaHei UI"
                                            elide: Text.ElideRight
                                        }
                                    }

                                    Text {
                                        Layout.preferredWidth: 44
                                        text: backend.classicModeEnabled ? "ON" : "OFF"
                                        color: backend.classicModeEnabled ? accentCyan : textMuted
                                        font.pixelSize: 11
                                        font.bold: true
                                        font.family: "Segoe UI"
                                        horizontalAlignment: Text.AlignRight
                                    }
                                }
                            }

                            Row {
                                width: parent.width
                                height: 38
                                spacing: 8

                                Rectangle {
                                    width: (parent.width - parent.spacing) / 2
                                    height: parent.height
                                    radius: 8
                                    property bool selected: backend.modelArmSize === "slim"
                                    color: selected ? Qt.rgba(0.545, 0.361, 0.965, 0.16) : Qt.rgba(1, 1, 1, 0.025)
                                    border.width: 1
                                    border.color: selected ? Qt.rgba(0.545, 0.361, 0.965, 0.56) : Qt.rgba(1, 1, 1, 0.08)

                                    Text {
                                        anchors.centerIn: parent
                                        text: "Alex · 细手臂"
                                        color: parent.selected ? "#DDD6FE" : textSecondary
                                        font.pixelSize: 12
                                        font.bold: true
                                        font.family: "Microsoft YaHei UI"
                                    }

                                    TapHandler { onTapped: backend.modelArmSize = "slim" }
                                }

                                Rectangle {
                                    width: (parent.width - parent.spacing) / 2
                                    height: parent.height
                                    radius: 8
                                    property bool selected: backend.modelArmSize === "wide"
                                    color: selected ? Qt.rgba(0.133, 0.827, 0.933, 0.15) : Qt.rgba(1, 1, 1, 0.025)
                                    border.width: 1
                                    border.color: selected ? Qt.rgba(0.133, 0.827, 0.933, 0.52) : Qt.rgba(1, 1, 1, 0.08)

                                    Text {
                                        anchors.centerIn: parent
                                        text: "Steve · 粗手臂"
                                        color: parent.selected ? "#CFFAFE" : textSecondary
                                        font.pixelSize: 12
                                        font.bold: true
                                        font.family: "Microsoft YaHei UI"
                                    }

                                    TapHandler { onTapped: backend.modelArmSize = "wide" }
                                }
                            }
                        }

                        Column {
                            id: hotkeySettingsContent
                            visible: settingsPageRestored.settingsTab === 1
                            width: parent.width
                            spacing: 12

                            property var previewBindings: [
                                { name: "启动注入", combo: "Ctrl + I", keys: ["Ctrl", "I"], enabled: true },
                                { name: "切换模型替换", combo: "Alt + M", keys: ["Alt", "M"], enabled: true },
                                { name: "经典方块人覆盖", combo: "Alt + C", keys: ["Alt", "C"], enabled: true },
                                { name: "日志窗口", combo: "F8", keys: ["F8"], enabled: true },
                                { name: "紧急停止", combo: "未绑定", keys: [], enabled: false }
                            ]

                            function keyBound(label) {
                                for (var i = 0; i < previewBindings.length; ++i) {
                                    if (!previewBindings[i].enabled)
                                        continue
                                    if (previewBindings[i].keys.indexOf(label) >= 0)
                                        return true
                                }
                                return false
                            }

                            function keyBindingText(label) {
                                var result = []
                                for (var i = 0; i < previewBindings.length; ++i) {
                                    if (!previewBindings[i].enabled)
                                        continue
                                    if (previewBindings[i].keys.indexOf(label) >= 0)
                                        result.push(previewBindings[i].combo)
                                }
                                return result.join(" / ")
                            }

                            Column {
                                width: parent.width
                                spacing: 3

                                Text {
                                    width: parent.width
                                    text: "快捷键"
                                    color: textPrimary
                                    font.pixelSize: 16
                                    font.bold: true
                                    font.family: "Microsoft YaHei UI"
                                    elide: Text.ElideRight
                                }

                                Text {
                                    width: parent.width
                                    text: "完整键盘预览 · 先看布局格式"
                                    color: textSecondary
                                    font.pixelSize: 11
                                    font.family: "Microsoft YaHei UI"
                                    elide: Text.ElideRight
                                }
                            }

                            Row {
                                width: parent.width
                                height: 24
                                spacing: 8

                                Repeater {
                                    model: ["Ctrl + I", "Alt + M", "Alt + C", "F8"]

                                    delegate: Rectangle {
                                        height: parent.height
                                        width: Math.max(62, comboText.implicitWidth + 18)
                                        radius: 6
                                        color: Qt.rgba(0.133, 0.827, 0.933, 0.075)
                                        border.width: 1
                                        border.color: Qt.rgba(0.133, 0.827, 0.933, 0.28)

                                        Text {
                                            id: comboText
                                            anchors.centerIn: parent
                                            text: modelData
                                            color: accentCyan
                                            font.pixelSize: 10
                                            font.bold: true
                                            font.family: "Segoe UI"
                                        }
                                    }
                                }
                            }

                            Item {
                                width: 1
                                height: 8
                            }

                            Repeater {
                                model: hotkeySettingsContent.previewBindings

                                delegate: Rectangle {
                                    width: parent.width
                                    height: 36
                                    radius: 8
                                    color: shortcutHover.hovered ? Qt.rgba(1, 1, 1, 0.050) : Qt.rgba(1, 1, 1, 0.024)
                                    border.width: 1
                                    border.color: shortcutHover.hovered ? Qt.rgba(0.545, 0.361, 0.965, 0.28) : Qt.rgba(1, 1, 1, 0.075)

                                    HoverHandler { id: shortcutHover }

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 12
                                        anchors.rightMargin: 10
                                        spacing: 10

                                        Text {
                                            Layout.fillWidth: true
                                            text: modelData.name
                                            color: textSecondary
                                            font.pixelSize: 12
                                            font.bold: true
                                            font.family: "Microsoft YaHei UI"
                                            elide: Text.ElideRight
                                        }

                                        Text {
                                            Layout.preferredWidth: 72
                                            text: modelData.combo
                                            color: modelData.enabled ? accentCyan : textMuted
                                            font.pixelSize: 10
                                            font.bold: modelData.enabled
                                            font.family: "Segoe UI"
                                            horizontalAlignment: Text.AlignRight
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                id: keyboardPreview
                                width: Math.min(596, parent.width)
                                height: 178
                                anchors.horizontalCenter: parent.horizontalCenter
                                radius: 10
                                color: Qt.rgba(0.012, 0.019, 0.032, 0.72)
                                border.width: 1
                                border.color: Qt.rgba(0.133, 0.827, 0.933, 0.24)

                                property real keyGap: 2
                                property real keyUnit: Math.min(23, Math.max(10, (width - 31) / 23.05))
                                property real keyHeight: 21
                                property var rows: [
                                    [
                                        { label: "Esc", u: 1 }, { spacer: true, u: 0.65 },
                                        { label: "F1", u: 1 }, { label: "F2", u: 1 }, { label: "F3", u: 1 }, { label: "F4", u: 1 }, { spacer: true, u: 0.35 },
                                        { label: "F5", u: 1 }, { label: "F6", u: 1 }, { label: "F7", u: 1 }, { label: "F8", u: 1 }, { spacer: true, u: 0.35 },
                                        { label: "F9", u: 1 }, { label: "F10", u: 1 }, { label: "F11", u: 1 }, { label: "F12", u: 1 }, { spacer: true, u: 0.55 },
                                        { label: "Prt", u: 1 }, { label: "Scr", u: 1 }, { label: "Pause", u: 1.25 }
                                    ],
                                    [
                                        { label: "`", u: 1 }, { label: "1", u: 1 }, { label: "2", u: 1 }, { label: "3", u: 1 }, { label: "4", u: 1 }, { label: "5", u: 1 }, { label: "6", u: 1 }, { label: "7", u: 1 }, { label: "8", u: 1 }, { label: "9", u: 1 }, { label: "0", u: 1 }, { label: "-", u: 1 }, { label: "=", u: 1 }, { label: "Back", u: 1.75 }, { spacer: true, u: 0.55 },
                                        { label: "Ins", u: 1 }, { label: "Home", u: 1 }, { label: "PgUp", u: 1 }, { spacer: true, u: 0.35 },
                                        { label: "Num", u: 1 }, { label: "/", u: 1 }, { label: "*", u: 1 }, { label: "-", u: 1 }
                                    ],
                                    [
                                        { label: "Tab", u: 1.45 }, { label: "Q", u: 1 }, { label: "W", u: 1 }, { label: "E", u: 1 }, { label: "R", u: 1 }, { label: "T", u: 1 }, { label: "Y", u: 1 }, { label: "U", u: 1 }, { label: "I", u: 1 }, { label: "O", u: 1 }, { label: "P", u: 1 }, { label: "[", u: 1 }, { label: "]", u: 1 }, { label: "\\", u: 1.3 }, { spacer: true, u: 0.55 },
                                        { label: "Del", u: 1 }, { label: "End", u: 1 }, { label: "PgDn", u: 1 }, { spacer: true, u: 0.35 },
                                        { label: "7", u: 1 }, { label: "8", u: 1 }, { label: "9", u: 1 }, { label: "+", u: 1 }
                                    ],
                                    [
                                        { label: "Caps", u: 1.72 }, { label: "A", u: 1 }, { label: "S", u: 1 }, { label: "D", u: 1 }, { label: "F", u: 1 }, { label: "G", u: 1 }, { label: "H", u: 1 }, { label: "J", u: 1 }, { label: "K", u: 1 }, { label: "L", u: 1 }, { label: ";", u: 1 }, { label: "'", u: 1 }, { label: "Enter", u: 2.05 }, { spacer: true, u: 4.0 },
                                        { label: "4", u: 1 }, { label: "5", u: 1 }, { label: "6", u: 1 }, { label: "+", u: 1 }
                                    ],
                                    [
                                        { label: "Shift", u: 2.25 }, { label: "Z", u: 1 }, { label: "X", u: 1 }, { label: "C", u: 1 }, { label: "V", u: 1 }, { label: "B", u: 1 }, { label: "N", u: 1 }, { label: "M", u: 1 }, { label: ",", u: 1 }, { label: ".", u: 1 }, { label: "/", u: 1 }, { label: "Shift", u: 2.35 }, { spacer: true, u: 1.55 },
                                        { label: "Up", u: 1 }, { spacer: true, u: 1.35 },
                                        { label: "1", u: 1 }, { label: "2", u: 1 }, { label: "3", u: 1 }, { label: "Enter", u: 1 }
                                    ],
                                    [
                                        { label: "Ctrl", u: 1.35 }, { label: "Win", u: 1.2 }, { label: "Alt", u: 1.25 }, { label: "Space", u: 6.2 }, { label: "Alt", u: 1.25 }, { label: "Fn", u: 1.15 }, { label: "Menu", u: 1.25 }, { label: "Ctrl", u: 1.35 }, { spacer: true, u: 0.65 },
                                        { label: "Left", u: 1 }, { label: "Down", u: 1 }, { label: "Right", u: 1 }, { spacer: true, u: 0.35 },
                                        { label: "0", u: 2.05 }, { label: ".", u: 1 }, { label: "Enter", u: 1 }
                                    ]
                                ]

                                Rectangle {
                                    anchors.fill: parent
                                    anchors.margins: 6
                                    radius: 8
                                    color: Qt.rgba(0.133, 0.827, 0.933, 0.022)
                                    border.width: 1
                                    border.color: Qt.rgba(0.545, 0.361, 0.965, 0.16)
                                }

                                Rectangle {
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.top: parent.top
                                    anchors.margins: 7
                                    height: 16
                                    radius: 7
                                    color: Qt.rgba(0.133, 0.827, 0.933, 0.050)
                                    border.width: 0
                                }

                                Rectangle {
                                    width: parent.width - 22
                                    height: 1
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.top: parent.top
                                    anchors.topMargin: 31
                                    color: Qt.rgba(0.133, 0.827, 0.933, 0.18)
                                }

                                Column {
                                    id: keyboardRows
                                    width: Math.min(565, Math.max(0, parent.width - 20))
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.top: parent.top
                                    anchors.topMargin: 15
                                    spacing: 3

                                    Repeater {
                                        model: keyboardPreview.rows

                                        delegate: Row {
                                            height: keyboardPreview.keyHeight
                                            spacing: keyboardPreview.keyGap
                                            property var rowKeys: modelData

                                            Repeater {
                                                model: rowKeys

                                                delegate: Item {
                                                    width: keyboardPreview.keyUnit * modelData.u
                                                    height: keyboardPreview.keyHeight

                                                    Rectangle {
                                                        anchors.fill: parent
                                                        visible: !modelData.spacer
                                                        radius: 4
                                                        property bool bound: hotkeySettingsContent.keyBound(modelData.label)
                                                        color: bound ? Qt.rgba(0.133, 0.827, 0.933, 0.13) : keyHover.hovered ? Qt.rgba(0.133, 0.827, 0.933, 0.07) : Qt.rgba(1, 1, 1, 0.022)
                                                        border.width: 1
                                                        border.color: bound ? Qt.rgba(0.133, 0.827, 0.933, 0.64) : keyHover.hovered ? Qt.rgba(0.133, 0.827, 0.933, 0.25) : Qt.rgba(1, 1, 1, 0.075)

                                                        Rectangle {
                                                            anchors.left: parent.left
                                                            anchors.right: parent.right
                                                            anchors.top: parent.top
                                                            anchors.margins: 1
                                                            height: 2
                                                            radius: 1
                                                            color: parent.bound ? Qt.rgba(0.133, 0.827, 0.933, 0.42) : Qt.rgba(1, 1, 1, 0.055)
                                                        }

                                                        Text {
                                                            anchors.centerIn: parent
                                                            text: modelData.label
                                                            color: parent.bound ? accentCyan : textSecondary
                                                            font.pixelSize: modelData.label.length > 4 ? 8 : 9
                                                            font.bold: false
                                                            font.family: "Segoe UI"
                                                            elide: Text.ElideRight
                                                        }

                                                        Rectangle {
                                                            visible: parent.bound
                                                            width: 4
                                                            height: 4
                                                            radius: 2
                                                            anchors.right: parent.right
                                                            anchors.top: parent.top
                                                            anchors.rightMargin: 3
                                                            anchors.topMargin: 3
                                                            color: accentCyan
                                                        }

                                                        HoverHandler { id: keyHover }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            Item {
                                width: 1
                                height: 42
                            }
                        }
                    }
                }

                FileDialog {
                    id: skinFileDialog
                    title: "选择皮肤 PNG"
                    nameFilters: ["PNG 图片 (*.png)"]
                    fileMode: FileDialog.OpenFile
                    onAccepted: {
                        const path = selectedFile.toString().replace(/^(file:\/{3})|(qrc:\/{2})/, "");
                        backend.setSkinPngPathFromQml(path)
                    }
                }

                Column {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.rightMargin: 4
                    anchors.bottomMargin: 2
                    width: Math.min(360, parent.width - 16)
                    spacing: 4

                    Text {
                        width: parent.width
                        horizontalAlignment: Text.AlignRight
                        text: "NoteBot Injector v3"
                        color: Qt.rgba(1, 1, 1, 0.32)
                        font.pixelSize: 11
                        font.family: "Segoe UI"
                        elide: Text.ElideRight
                    }

                    Text {
                        width: parent.width
                        horizontalAlignment: Text.AlignRight
                        text: "由 Claude/GPT/DeepSeek 协助维护 · 最终方向由帕帕确认"
                        color: Qt.rgba(1, 1, 1, 0.22)
                        font.pixelSize: 10
                        font.family: "Microsoft YaHei UI"
                        elide: Text.ElideRight
                    }

                    Text {
                        width: parent.width
                        horizontalAlignment: Text.AlignRight
                        text: "技术实现、界面迭代与构建流程由 AI 代理持续参与"
                        color: Qt.rgba(1, 1, 1, 0.18)
                        font.pixelSize: 10
                        font.family: "Microsoft YaHei UI"
                        elide: Text.ElideRight
                    }

                    Text {
                        width: parent.width
                        horizontalAlignment: Text.AlignRight
                        text: "非 Mojang 或 Microsoft 官方产品"
                        color: Qt.rgba(1, 1, 1, 0.16)
                        font.pixelSize: 10
                        font.family: "Microsoft YaHei UI"
                        elide: Text.ElideRight
                    }
                }
            }
    }
}
    }
}
