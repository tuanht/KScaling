// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#include "MockDisplayBackend.h"

#include <QTest>

class MockDisplayBackendTest : public QObject
{
    Q_OBJECT

private slots:
    void leftoverCustom_isNotSelectableBeforeReload();
    void applyCustom_reissuesIdsAfterReload();
    void setCurrentModeId_refusesPrePhaseASnapshotId();
    void failPhaseA_doesNotSwitchOrRegister();
    void failPhaseB_restoresPreferredAndScale();
    void applyCustom_setsCanvasAtScale200();
    void leftoverCustom_skipsPhaseAAndSwitchesAfterReload();

private:
    static OutputSnapshot connectedDp3(qreal scale = 1.0);
};

OutputSnapshot MockDisplayBackendTest::connectedDp3(qreal scale)
{
    OutputSnapshot snapshot;
    snapshot.name = QStringLiteral("DP-3");
    snapshot.connected = true;
    snapshot.customModesCapable = true;
    snapshot.scale = scale;
    snapshot.native = Mode{{2560, 1440}, 164.96, {}};
    snapshot.current = snapshot.native;
    return snapshot;
}

void MockDisplayBackendTest::leftoverCustom_isNotSelectableBeforeReload()
{
    MockDisplayBackend mock;
    OutputSnapshot snapshot = connectedDp3();
    snapshot.customModes.append(SizeHz{{4096, 2304}, 59.93});
    mock.injectOutput(snapshot);

    QVERIFY(!mock.setCurrentModeId(QStringLiteral("DP-3"), QStringLiteral("2")).ok);

    const OutputSnapshot listed = mock.list().snapshots.at(0);
    QCOMPARE(listed.current.size.w, 2560);
    QCOMPARE(listed.current.size.h, 1440);
    QCOMPARE(listed.customModes.size(), 1);
}

void MockDisplayBackendTest::applyCustom_reissuesIdsAfterReload()
{
    MockDisplayBackend mock;
    mock.injectOutput(connectedDp3());

    const QString preNativeId = mock.list().snapshots.at(0).native.id;
    QVERIFY(!preNativeId.isEmpty());

    const Result applied = mock.applyCustom(QStringLiteral("DP-3"), Size{4096, 2304}, 60.0);
    QVERIFY(applied.ok);

    const OutputSnapshot listed = mock.list().snapshots.at(0);
    QVERIFY(listed.native.id != preNativeId);
    QVERIFY(listed.current.id != preNativeId);
}

void MockDisplayBackendTest::setCurrentModeId_refusesPrePhaseASnapshotId()
{
    MockDisplayBackend mock;
    mock.injectOutput(connectedDp3());

    const QString preNativeId = mock.list().snapshots.at(0).native.id;
    QVERIFY(mock.applyCustom(QStringLiteral("DP-3"), Size{4096, 2304}, 60.0).ok);

    const Result stale = mock.setCurrentModeId(QStringLiteral("DP-3"), preNativeId);
    QVERIFY(!stale.ok);

    const OutputSnapshot listed = mock.list().snapshots.at(0);
    QCOMPARE(listed.current.size.w, 4096);
    QCOMPARE(listed.current.size.h, 2304);
}

void MockDisplayBackendTest::failPhaseA_doesNotSwitchOrRegister()
{
    MockDisplayBackend mock;
    mock.injectOutput(connectedDp3(1.5));
    mock.setFailPhaseA(true, QStringLiteral("phase A rejected"));

    const QString preNativeId = mock.list().snapshots.at(0).native.id;
    const Result applied = mock.applyCustom(QStringLiteral("DP-3"), Size{4096, 2304}, 60.0);
    QVERIFY(!applied.ok);
    QCOMPARE(applied.error, QStringLiteral("phase A rejected"));

    const OutputSnapshot listed = mock.list().snapshots.at(0);
    QCOMPARE(listed.current.id, preNativeId);
    QCOMPARE(listed.current.size.w, 2560);
    QCOMPARE(listed.scale, 1.5);
    QCOMPARE(listed.customModes.size(), 0);
}

void MockDisplayBackendTest::failPhaseB_restoresPreferredAndScale()
{
    MockDisplayBackend mock;
    mock.injectOutput(connectedDp3(1.5));
    mock.setFailPhaseB(true, QStringLiteral("phase B rejected"));

    const Result applied = mock.applyCustom(QStringLiteral("DP-3"), Size{4096, 2304}, 60.0);
    QVERIFY(!applied.ok);
    QCOMPARE(applied.error, QStringLiteral("phase B rejected"));

    const OutputSnapshot listed = mock.list().snapshots.at(0);
    QCOMPARE(listed.current.size.w, 2560);
    QCOMPARE(listed.current.size.h, 1440);
    QCOMPARE(listed.scale, 1.5);
    QCOMPARE(listed.customModes.size(), 1);
}

void MockDisplayBackendTest::applyCustom_setsCanvasAtScale200()
{
    MockDisplayBackend mock;
    mock.injectOutput(connectedDp3(1.0));

    QVERIFY(mock.applyCustom(QStringLiteral("DP-3"), Size{4096, 2304}, 59.93).ok);

    const OutputSnapshot listed = mock.list().snapshots.at(0);
    QCOMPARE(listed.current.size.w, 4096);
    QCOMPARE(listed.current.size.h, 2304);
    QCOMPARE(listed.scale, 2.00);
    QCOMPARE(listed.name, QStringLiteral("DP-3"));
}

void MockDisplayBackendTest::leftoverCustom_skipsPhaseAAndSwitchesAfterReload()
{
    MockDisplayBackend mock;
    OutputSnapshot snapshot = connectedDp3();
    snapshot.customModes.append(SizeHz{{4096, 2304}, 59.93});
    mock.injectOutput(snapshot);
    mock.setFailPhaseA(true, QStringLiteral("phase A rejected"));

    QVERIFY(mock.applyCustom(QStringLiteral("DP-3"), Size{4096, 2304}, 60.0).ok);

    const OutputSnapshot listed = mock.list().snapshots.at(0);
    QCOMPARE(listed.current.size.w, 4096);
    QCOMPARE(listed.current.size.h, 2304);
    QCOMPARE(listed.scale, 2.00);
}

QTEST_APPLESS_MAIN(MockDisplayBackendTest)
#include "MockDisplayBackendTest.moc"
