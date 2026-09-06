// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#include "ApplyService.h"
#include "MockDisplayBackend.h"
#include "Settings.h"

#include <QCoreApplication>
#include <QFile>
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

class ApplyOrchestratorTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void reuseExistingCustom_bySizeAndRoundedHz_doesNotDuplicate();
    void native_comesFromPreferred_notCurrentCanvas();
    void failedPhaseB_restoresPreferredAndScaleBeforeAttempt_notOriginalScale();
    void apply_neverSwitchesUsingPrePhaseAModeId();
    void incapableCustomModes_noModeOrScaleChange_mapsToExit1();
    void compositorReject_doesNotRetryOtherHz();
    void originalScale_writeOnceOnFirstSuccessfulApply();
    void persistSavedProfile_onlyOnSuccess();

private:
    static OutputSnapshot connectedDp3(qreal scale = 1.0);
};

void ApplyOrchestratorTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setApplicationName(QStringLiteral("kscaling"));
}

void ApplyOrchestratorTest::cleanup()
{
    QFile::remove(Settings::filePath());
}

OutputSnapshot ApplyOrchestratorTest::connectedDp3(qreal scale)
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

void ApplyOrchestratorTest::reuseExistingCustom_bySizeAndRoundedHz_doesNotDuplicate()
{
    MockDisplayBackend mock;
    OutputSnapshot snapshot = connectedDp3(1.0);
    snapshot.customModes.append(SizeHz{{4096, 2304}, 59.7});
    mock.injectOutput(snapshot);
    mock.setFailPhaseA(true, QStringLiteral("phase A rejected"));

    ApplyService service(mock);
    const ApplyService::Result result = service.apply(QStringLiteral("perfect"), QStringLiteral("DP-3"));
    QVERIFY(result.ok);
    QCOMPARE(result.exitCode, 0);

    const OutputSnapshot listed = mock.list().snapshots.at(0);
    QCOMPARE(listed.name, QStringLiteral("DP-3"));
    QCOMPARE(listed.current.size.w, 4096);
    QCOMPARE(listed.current.size.h, 2304);
    QCOMPARE(listed.scale, 2.00);
    QCOMPARE(listed.customModes.size(), 1);
    QCOMPARE(listed.customModes.at(0).size.w, 4096);
    QCOMPARE(listed.customModes.at(0).size.h, 2304);
    QCOMPARE(qRound(listed.customModes.at(0).hz), 60);

    const Settings::LoadResult loaded = Settings::load();
    QVERIFY(loaded.ok);
    QVERIFY(loaded.outputs.contains(QStringLiteral("DP-3")));
    const SavedProfile& profile = loaded.outputs.value(QStringLiteral("DP-3"));
    QCOMPARE(profile.preset, QStringLiteral("perfect"));
    QCOMPARE(profile.mode, QSize(4096, 2304));
    QCOMPARE(profile.hz, 60.0);
    QCOMPARE(profile.originalScale, 1.0);
}

void ApplyOrchestratorTest::native_comesFromPreferred_notCurrentCanvas()
{
    MockDisplayBackend mock;
    OutputSnapshot snapshot = connectedDp3(2.00);
    snapshot.current = Mode{{4096, 2304}, 59.93, {}};
    snapshot.customModes.append(SizeHz{{4096, 2304}, 59.93});
    mock.injectOutput(snapshot);

    ApplyService service(mock);
    const ApplyService::Result result = service.apply(QStringLiteral("comfort"), QStringLiteral("DP-3"));
    QVERIFY(result.ok);

    const OutputSnapshot listed = mock.list().snapshots.at(0);
    QCOMPARE(listed.native.size.w, 2560);
    QCOMPARE(listed.native.size.h, 1440);
    QCOMPARE(listed.current.size.w, 3200);
    QCOMPARE(listed.current.size.h, 1800);
    QCOMPARE(listed.scale, 2.00);
}

void ApplyOrchestratorTest::failedPhaseB_restoresPreferredAndScaleBeforeAttempt_notOriginalScale()
{
    MockDisplayBackend mock;
    mock.injectOutput(connectedDp3(1.0));

    ApplyService service(mock);
    QVERIFY(service.apply(QStringLiteral("perfect"), QStringLiteral("DP-3")).ok);

    const OutputSnapshot afterFirst = mock.list().snapshots.at(0);
    QCOMPARE(afterFirst.scale, 2.00);
    QCOMPARE(afterFirst.current.size.w, 4096);

    mock.setFailPhaseB(true, QStringLiteral("phase B rejected"));
    const ApplyService::Result failed = service.apply(QStringLiteral("comfort"), QStringLiteral("DP-3"));
    QVERIFY(!failed.ok);
    QCOMPARE(failed.exitCode, 2);

    const OutputSnapshot listed = mock.list().snapshots.at(0);
    QCOMPARE(listed.current.size.w, 2560);
    QCOMPARE(listed.current.size.h, 1440);
    QCOMPARE(listed.scale, 2.00);

    const SavedProfile profile = Settings::load().outputs.value(QStringLiteral("DP-3"));
    QCOMPARE(profile.preset, QStringLiteral("perfect"));
    QCOMPARE(profile.mode, QSize(4096, 2304));
    QCOMPARE(profile.originalScale, 1.0);
}

void ApplyOrchestratorTest::apply_neverSwitchesUsingPrePhaseAModeId()
{
    MockDisplayBackend mock;
    mock.injectOutput(connectedDp3(1.0));

    const QString prePhaseAId = mock.list().snapshots.at(0).native.id;
    QVERIFY(!prePhaseAId.isEmpty());

    ApplyService service(mock);
    QVERIFY(service.apply(QStringLiteral("perfect"), QStringLiteral("DP-3")).ok);

    const OutputSnapshot listed = mock.list().snapshots.at(0);
    QVERIFY(listed.current.id != prePhaseAId);
    QVERIFY(listed.native.id != prePhaseAId);
    QCOMPARE(listed.current.size.w, 4096);
    QCOMPARE(listed.current.size.h, 2304);
    QCOMPARE(listed.scale, 2.00);
}

void ApplyOrchestratorTest::incapableCustomModes_noModeOrScaleChange_mapsToExit1()
{
    MockDisplayBackend mock;
    OutputSnapshot snapshot = connectedDp3(1.5);
    snapshot.customModesCapable = false;
    mock.injectOutput(snapshot);

    const QString modeId = mock.list().snapshots.at(0).current.id;

    ApplyService service(mock);
    const ApplyService::Result result = service.apply(QStringLiteral("perfect"), QStringLiteral("DP-3"));
    QVERIFY(!result.ok);
    QCOMPARE(result.exitCode, 1);

    const OutputSnapshot listed = mock.list().snapshots.at(0);
    QCOMPARE(listed.current.id, modeId);
    QCOMPARE(listed.current.size.w, 2560);
    QCOMPARE(listed.current.size.h, 1440);
    QCOMPARE(listed.scale, 1.5);
    QCOMPARE(listed.customModes.size(), 0);

    QVERIFY(Settings::load().outputs.isEmpty());
}

void ApplyOrchestratorTest::compositorReject_doesNotRetryOtherHz()
{
    MockDisplayBackend mock;
    mock.injectOutput(connectedDp3(1.0));
    mock.setFailPhaseA(true, QStringLiteral("rejected"));

    SpyDisplayBackend spy(mock);
    ApplyService service(spy);
    const ApplyService::Result result = service.apply(QStringLiteral("perfect"), QStringLiteral("DP-3"));
    QVERIFY(!result.ok);
    QCOMPARE(result.exitCode, 2);

    QCOMPARE(spy.applyCustomCalls, 1);
    QCOMPARE(spy.lastName, QStringLiteral("DP-3"));
    QCOMPARE(spy.lastCanvas.w, 4096);
    QCOMPARE(spy.lastCanvas.h, 2304);
    QCOMPARE(spy.lastHz, 60.0);
    QCOMPARE(spy.lastScale, 2.00);

    const OutputSnapshot listed = mock.list().snapshots.at(0);
    QCOMPARE(listed.current.size.w, 2560);
    QCOMPARE(listed.current.size.h, 1440);
    QCOMPARE(listed.scale, 1.0);
    QVERIFY(Settings::load().outputs.isEmpty());
}

void ApplyOrchestratorTest::originalScale_writeOnceOnFirstSuccessfulApply()
{
    MockDisplayBackend mock;
    mock.injectOutput(connectedDp3(1.5));

    ApplyService service(mock);
    QVERIFY(service.apply(QStringLiteral("perfect"), QStringLiteral("DP-3")).ok);
    QCOMPARE(Settings::load().outputs.value(QStringLiteral("DP-3")).originalScale, 1.5);

    QVERIFY(service.apply(QStringLiteral("comfort"), QStringLiteral("DP-3")).ok);

    const SavedProfile profile = Settings::load().outputs.value(QStringLiteral("DP-3"));
    QCOMPARE(profile.preset, QStringLiteral("comfort"));
    QCOMPARE(profile.mode, QSize(3200, 1800));
    QCOMPARE(profile.hz, 120.0);
    QCOMPARE(profile.originalScale, 1.5);
}

void ApplyOrchestratorTest::persistSavedProfile_onlyOnSuccess()
{
    SavedProfile existing;
    existing.preset = QStringLiteral("comfort");
    existing.mode = QSize(3200, 1800);
    existing.hz = 120;
    existing.originalScale = 1.25;
    Settings::ProfileMap outputs;
    outputs.insert(QStringLiteral("DP-3"), existing);
    QVERIFY(Settings::save(outputs).ok);

    MockDisplayBackend mock;
    mock.injectOutput(connectedDp3(1.0));
    mock.setFailPhaseA(true, QStringLiteral("rejected"));

    ApplyService service(mock);
    QVERIFY(!service.apply(QStringLiteral("perfect"), QStringLiteral("DP-3")).ok);

    const SavedProfile profile = Settings::load().outputs.value(QStringLiteral("DP-3"));
    QCOMPARE(profile.preset, QStringLiteral("comfort"));
    QCOMPARE(profile.mode, QSize(3200, 1800));
    QCOMPARE(profile.hz, 120.0);
    QCOMPARE(profile.originalScale, 1.25);
}

QTEST_GUILESS_MAIN(ApplyOrchestratorTest)
#include "ApplyOrchestratorTest.moc"
