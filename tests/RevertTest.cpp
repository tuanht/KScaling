// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#include "ApplyService.h"
#include "MockDisplayBackend.h"
#include "Settings.h"

#include <QCoreApplication>
#include <QFile>
#include <QStandardPaths>
#include <QTest>

class RevertTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void applyPerfectThenComfort_originalScaleIsFirstApplyPrior_not200();
    void revert_restoresPreferredModeAndOriginalScale();
    void revert_doesNotClearLastSuccessfulPresetModeHz();
    void revert_noPriorApply_restoresPreferred_reportsNoSavedScale_leavesScale();
    void revert_identityIsConnectorName();

private:
    static OutputSnapshot connectedDp3(qreal scale = 1.0);
    static OutputSnapshot connectedHdmi(qreal scale = 1.0);
};

void RevertTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setApplicationName(QStringLiteral("kscaling"));
}

void RevertTest::cleanup()
{
    QFile::remove(Settings::filePath());
}

OutputSnapshot RevertTest::connectedDp3(qreal scale)
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

OutputSnapshot RevertTest::connectedHdmi(qreal scale)
{
    OutputSnapshot snapshot;
    snapshot.name = QStringLiteral("HDMI-A-1");
    snapshot.connected = true;
    snapshot.customModesCapable = true;
    snapshot.scale = scale;
    snapshot.native = Mode{{1920, 1080}, 60.00, {}};
    snapshot.current = snapshot.native;
    return snapshot;
}

void RevertTest::applyPerfectThenComfort_originalScaleIsFirstApplyPrior_not200()
{
    MockDisplayBackend mock;
    mock.injectOutput(connectedDp3(1.5));

    ApplyService service(mock);
    QVERIFY(service.apply(QStringLiteral("perfect"), QStringLiteral("DP-3")).ok);
    QVERIFY(service.apply(QStringLiteral("comfort"), QStringLiteral("DP-3")).ok);

    const SavedProfile profile = Settings::load().outputs.value(QStringLiteral("DP-3"));
    QCOMPARE(profile.originalScale, 1.5);
    QVERIFY(profile.originalScale != 2.00);

    const OutputSnapshot listed = mock.list().snapshots.at(0);
    QCOMPARE(listed.scale, 2.00);
    QCOMPARE(listed.current.size.w, 3200);
    QCOMPARE(listed.current.size.h, 1800);
}

void RevertTest::revert_restoresPreferredModeAndOriginalScale()
{
    MockDisplayBackend mock;
    mock.injectOutput(connectedDp3(1.5));

    ApplyService service(mock);
    QVERIFY(service.apply(QStringLiteral("perfect"), QStringLiteral("DP-3")).ok);
    QVERIFY(service.apply(QStringLiteral("comfort"), QStringLiteral("DP-3")).ok);

    const ApplyService::Result result = service.revert(QStringLiteral("DP-3"));
    QVERIFY(result.ok);
    QCOMPARE(result.exitCode, 0);

    const OutputSnapshot listed = mock.list().snapshots.at(0);
    QCOMPARE(listed.current.size.w, 2560);
    QCOMPARE(listed.current.size.h, 1440);
    QCOMPARE(listed.scale, 1.5);
}

void RevertTest::revert_doesNotClearLastSuccessfulPresetModeHz()
{
    MockDisplayBackend mock;
    mock.injectOutput(connectedDp3(1.5));

    ApplyService service(mock);
    QVERIFY(service.apply(QStringLiteral("perfect"), QStringLiteral("DP-3")).ok);
    QVERIFY(service.apply(QStringLiteral("comfort"), QStringLiteral("DP-3")).ok);

    QFile beforeFile(Settings::filePath());
    QVERIFY(beforeFile.open(QIODevice::ReadOnly));
    const QByteArray before = beforeFile.readAll();
    beforeFile.close();
    QVERIFY(!before.isEmpty());

    QVERIFY(service.revert(QStringLiteral("DP-3")).ok);

    QFile afterFile(Settings::filePath());
    QVERIFY(afterFile.open(QIODevice::ReadOnly));
    const QByteArray after = afterFile.readAll();
    QCOMPARE(after, before);

    const SavedProfile profile = Settings::load().outputs.value(QStringLiteral("DP-3"));
    QCOMPARE(profile.preset, QStringLiteral("comfort"));
    QCOMPARE(profile.mode, QSize(3200, 1800));
    QCOMPARE(profile.hz, 120.0);
    QCOMPARE(profile.originalScale, 1.5);
}

void RevertTest::revert_noPriorApply_restoresPreferred_reportsNoSavedScale_leavesScale()
{
    MockDisplayBackend mock;
    OutputSnapshot snapshot = connectedDp3(1.75);
    snapshot.current = Mode{{4096, 2304}, 59.93, {}};
    snapshot.customModes.append(SizeHz{{4096, 2304}, 59.93});
    mock.injectOutput(snapshot);

    ApplyService service(mock);
    const ApplyService::Result result = service.revert(QStringLiteral("DP-3"));
    QVERIFY(result.ok);
    QCOMPARE(result.exitCode, 0);
    QVERIFY(result.error.contains(QStringLiteral("no saved prior scale")));

    const OutputSnapshot listed = mock.list().snapshots.at(0);
    QCOMPARE(listed.current.size.w, 2560);
    QCOMPARE(listed.current.size.h, 1440);
    QCOMPARE(listed.scale, 1.75);

    QVERIFY(Settings::load().outputs.isEmpty());
}

void RevertTest::revert_identityIsConnectorName()
{
    MockDisplayBackend mock;
    mock.injectOutput(connectedDp3(1.5));
    mock.injectOutput(connectedHdmi(1.0));

    ApplyService service(mock);
    QVERIFY(service.apply(QStringLiteral("perfect"), QStringLiteral("DP-3")).ok);
    QVERIFY(service.revert(QStringLiteral("DP-3")).ok);

    const ListResult listed = mock.list();
    QCOMPARE(listed.snapshots.size(), 2);
    QCOMPARE(listed.snapshots.at(0).name, QStringLiteral("DP-3"));
    QCOMPARE(listed.snapshots.at(0).current.size.w, 2560);
    QCOMPARE(listed.snapshots.at(0).current.size.h, 1440);
    QCOMPARE(listed.snapshots.at(0).scale, 1.5);
    QCOMPARE(listed.snapshots.at(1).name, QStringLiteral("HDMI-A-1"));
    QCOMPARE(listed.snapshots.at(1).current.size.w, 1920);
    QCOMPARE(listed.snapshots.at(1).current.size.h, 1080);
    QCOMPARE(listed.snapshots.at(1).scale, 1.0);

    const Settings::LoadResult loaded = Settings::load();
    QVERIFY(loaded.ok);
    QCOMPARE(loaded.outputs.keys(), QStringList{QStringLiteral("DP-3")});
    QVERIFY(!loaded.outputs.contains(listed.snapshots.at(0).native.id));
    QVERIFY(!loaded.outputs.contains(listed.snapshots.at(0).current.id));
}

QTEST_GUILESS_MAIN(RevertTest)
#include "RevertTest.moc"
