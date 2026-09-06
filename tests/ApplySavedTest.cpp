// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#include "ApplyService.h"
#include "MockDisplayBackend.h"
#include "Settings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTest>

class SpyDisplayBackend : public DisplayBackend
{
public:
    explicit SpyDisplayBackend(DisplayBackend& inner)
        : m_inner(inner)
    {
    }

    int applyCustomCalls = 0;
    ConnectorName lastName;
    Size lastCanvas;
    qreal lastHz = 0;
    qreal lastScale = 0;
    QList<ConnectorName> applyCustomNames;

    ListResult list() override
    {
        return m_inner.list();
    }

    Result applyCustom(const ConnectorName& name,
                       Size canvas,
                       qreal hz,
                       qreal scale = 2.00) override
    {
        ++applyCustomCalls;
        lastName = name;
        lastCanvas = canvas;
        lastHz = hz;
        lastScale = scale;
        applyCustomNames.append(name);
        return m_inner.applyCustom(name, canvas, hz, scale);
    }

    Result revert(const ConnectorName& name,
                  std::optional<qreal> originalScale = std::nullopt) override
    {
        return m_inner.revert(name, originalScale);
    }

private:
    DisplayBackend& m_inner;
};

class ApplySavedTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void applyPerfect_revert_applySaved_restoresPerfect();
    void applySaved_skipsDisconnected_withoutFailingOthers();
    void applySaved_noSavedProfiles_exits0_nothingApplied();
    void applySaved_usesModeHzFirst_notRecomputedPreset();
    void applySaved_recomputesFromPresetAndNative_whenModeHzMissing();

private:
    static OutputSnapshot connectedDp3(qreal scale = 1.0);
    static OutputSnapshot disconnectedHdmi();
    static void writeProfilesJson(const QByteArray& json);
};

void ApplySavedTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setApplicationName(QStringLiteral("kscaling"));
}

void ApplySavedTest::cleanup()
{
    QFile::remove(Settings::filePath());
}

OutputSnapshot ApplySavedTest::connectedDp3(qreal scale)
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

OutputSnapshot ApplySavedTest::disconnectedHdmi()
{
    OutputSnapshot snapshot;
    snapshot.name = QStringLiteral("HDMI-A-1");
    snapshot.connected = false;
    snapshot.customModesCapable = true;
    snapshot.scale = 1.0;
    snapshot.native = Mode{{1920, 1080}, 60.00, {}};
    snapshot.current = snapshot.native;
    return snapshot;
}

void ApplySavedTest::writeProfilesJson(const QByteArray& json)
{
    QVERIFY(QDir().mkpath(QFileInfo(Settings::filePath()).absolutePath()));
    QFile file(Settings::filePath());
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QCOMPARE(file.write(json), qsizetype(json.size()));
}

void ApplySavedTest::applyPerfect_revert_applySaved_restoresPerfect()
{
    MockDisplayBackend mock;
    mock.injectOutput(connectedDp3(1.5));

    ApplyService service(mock);
    QVERIFY(service.apply(QStringLiteral("perfect"), QStringLiteral("DP-3")).ok);
    QVERIFY(service.revert(QStringLiteral("DP-3")).ok);

    const OutputSnapshot afterRevert = mock.list().snapshots.at(0);
    QCOMPARE(afterRevert.current.size.w, 2560);
    QCOMPARE(afterRevert.current.size.h, 1440);
    QCOMPARE(afterRevert.scale, 1.5);

    mock.injectOutput(connectedDp3(1.5));

    SpyDisplayBackend spy(mock);
    ApplyService restore(spy);
    const ApplyService::Result result = restore.applySaved();
    QVERIFY(result.ok);
    QCOMPARE(result.exitCode, 0);
    QCOMPARE(result.applied.size(), 1);
    QCOMPARE(result.applied.at(0).connector, QStringLiteral("DP-3"));
    QCOMPARE(result.applied.at(0).preset, QStringLiteral("perfect"));
    QCOMPARE(result.applied.at(0).canvas.w, 4096);
    QCOMPARE(result.applied.at(0).canvas.h, 2304);
    QCOMPARE(result.applied.at(0).hz, 60.0);
    QCOMPARE(result.applied.at(0).scale, 2.00);

    QCOMPARE(spy.applyCustomCalls, 1);
    QCOMPARE(spy.lastName, QStringLiteral("DP-3"));
    QCOMPARE(spy.lastCanvas.w, 4096);
    QCOMPARE(spy.lastCanvas.h, 2304);
    QCOMPARE(spy.lastHz, 60.0);
    QCOMPARE(spy.lastScale, 2.00);

    const OutputSnapshot listed = mock.list().snapshots.at(0);
    QCOMPARE(listed.current.size.w, 4096);
    QCOMPARE(listed.current.size.h, 2304);
    QCOMPARE(listed.scale, 2.00);
    QCOMPARE(listed.customModes.size(), 1);
    QCOMPARE(listed.customModes.at(0).size.w, 4096);
    QCOMPARE(listed.customModes.at(0).size.h, 2304);
    QCOMPARE(qRound(listed.customModes.at(0).hz), 60);
}

void ApplySavedTest::applySaved_skipsDisconnected_withoutFailingOthers()
{
    MockDisplayBackend mock;
    mock.injectOutput(connectedDp3(1.0));
    mock.injectOutput(disconnectedHdmi());

    ApplyService service(mock);
    QVERIFY(service.apply(QStringLiteral("perfect"), QStringLiteral("DP-3")).ok);
    QVERIFY(service.revert(QStringLiteral("DP-3")).ok);

    SavedProfile hdmiProfile;
    hdmiProfile.preset = QStringLiteral("comfort");
    hdmiProfile.mode = QSize(3072, 1728);
    hdmiProfile.hz = 120;
    hdmiProfile.originalScale = 1.0;
    Settings::ProfileMap outputs = Settings::load().outputs;
    outputs.insert(QStringLiteral("HDMI-A-1"), hdmiProfile);
    QVERIFY(Settings::save(outputs).ok);

    SpyDisplayBackend spy(mock);
    ApplyService restore(spy);
    const ApplyService::Result result = restore.applySaved();
    QVERIFY(result.ok);
    QCOMPARE(result.exitCode, 0);
    QCOMPARE(result.applied.size(), 1);
    QCOMPARE(result.applied.at(0).connector, QStringLiteral("DP-3"));

    QCOMPARE(spy.applyCustomCalls, 1);
    QCOMPARE(spy.applyCustomNames, QList<ConnectorName>{QStringLiteral("DP-3")});
    QCOMPARE(spy.lastName, QStringLiteral("DP-3"));
    QCOMPARE(spy.lastCanvas.w, 4096);
    QCOMPARE(spy.lastCanvas.h, 2304);

    const ListResult listed = mock.list();
    QCOMPARE(listed.snapshots.size(), 2);
    QCOMPARE(listed.snapshots.at(0).name, QStringLiteral("DP-3"));
    QCOMPARE(listed.snapshots.at(0).current.size.w, 4096);
    QCOMPARE(listed.snapshots.at(0).current.size.h, 2304);
    QCOMPARE(listed.snapshots.at(0).scale, 2.00);
    QCOMPARE(listed.snapshots.at(1).name, QStringLiteral("HDMI-A-1"));
    QCOMPARE(listed.snapshots.at(1).connected, false);
    QCOMPARE(listed.snapshots.at(1).current.size.w, 1920);
    QCOMPARE(listed.snapshots.at(1).current.size.h, 1080);
    QCOMPARE(listed.snapshots.at(1).scale, 1.0);
}

void ApplySavedTest::applySaved_noSavedProfiles_exits0_nothingApplied()
{
    MockDisplayBackend mock;
    mock.injectOutput(connectedDp3(1.0));

    SpyDisplayBackend spy(mock);
    ApplyService service(spy);
    const ApplyService::Result result = service.applySaved();
    QVERIFY(result.ok);
    QCOMPARE(result.exitCode, 0);
    QVERIFY(result.applied.isEmpty());
    QCOMPARE(spy.applyCustomCalls, 0);

    const OutputSnapshot listed = mock.list().snapshots.at(0);
    QCOMPARE(listed.current.size.w, 2560);
    QCOMPARE(listed.current.size.h, 1440);
    QCOMPARE(listed.scale, 1.0);
}

void ApplySavedTest::applySaved_usesModeHzFirst_notRecomputedPreset()
{
    MockDisplayBackend mock;
    mock.injectOutput(connectedDp3(1.0));

    SavedProfile profile;
    profile.preset = QStringLiteral("perfect");
    profile.mode = QSize(3200, 1800);
    profile.hz = 60;
    profile.originalScale = 1.0;
    Settings::ProfileMap outputs;
    outputs.insert(QStringLiteral("DP-3"), profile);
    QVERIFY(Settings::save(outputs).ok);

    SpyDisplayBackend spy(mock);
    ApplyService service(spy);
    const ApplyService::Result result = service.applySaved();
    QVERIFY(result.ok);
    QCOMPARE(result.exitCode, 0);

    QCOMPARE(spy.applyCustomCalls, 1);
    QCOMPARE(spy.lastCanvas.w, 3200);
    QCOMPARE(spy.lastCanvas.h, 1800);
    QCOMPARE(spy.lastHz, 60.0);
    QCOMPARE(spy.lastScale, 2.00);

    const OutputSnapshot listed = mock.list().snapshots.at(0);
    QCOMPARE(listed.current.size.w, 3200);
    QCOMPARE(listed.current.size.h, 1800);
}

void ApplySavedTest::applySaved_recomputesFromPresetAndNative_whenModeHzMissing()
{
    MockDisplayBackend mock;
    mock.injectOutput(connectedDp3(1.0));

    writeProfilesJson(R"({
        "version": 1,
        "outputs": {
            "DP-3": {
                "preset": "perfect",
                "originalScale": 1.0
            }
        }
    })");

    SpyDisplayBackend spy(mock);
    ApplyService service(spy);
    const ApplyService::Result result = service.applySaved();
    QVERIFY(result.ok);
    QCOMPARE(result.exitCode, 0);

    QCOMPARE(spy.applyCustomCalls, 1);
    QCOMPARE(spy.lastCanvas.w, 4096);
    QCOMPARE(spy.lastCanvas.h, 2304);
    QCOMPARE(spy.lastHz, 60.0);
    QCOMPARE(spy.lastScale, 2.00);

    const OutputSnapshot listed = mock.list().snapshots.at(0);
    QCOMPARE(listed.current.size.w, 4096);
    QCOMPARE(listed.current.size.h, 2304);
}

QTEST_GUILESS_MAIN(ApplySavedTest)
#include "ApplySavedTest.moc"
