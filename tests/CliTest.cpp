// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#include "cli/Cli.h"

#include <QTest>

class CliTest : public QObject
{
    Q_OBJECT

private slots:
    void parse_applyPerfect();
    void parse_applyMaxSpace();
    void parse_applyComfort();
    void parse_applyLarge();
    void parse_applyWithOutput();
    void parse_outputBeforeApply();
    void parse_revert();
    void parse_revertWithOutput();
    void parse_list();
    void parse_applySaved();
    void parse_help();
    void parse_unknownPreset_isUsageError();
    void parse_applyMissingPreset_isUsageError();
    void parse_outputWithList_isUsageError();
    void parse_outputWithApplySaved_isUsageError();
    void parse_applyAndList_isUsageError();
    void parse_unknownFlag_isUsageError();

    void formatApplySaved_empty_isNothingApplied();
    void formatApplySaved_printsConnectorPresetCanvasScale();

    void resolve_zeroConnected_isExit3();
    void resolve_oneConnected_omittedOutput();
    void resolve_oneConnected_matchingOutput();
    void resolve_oneConnected_mismatch_isExit4();
    void resolve_manyConnected_omittedOutput_isExit4();
    void resolve_manyConnected_unknown_isExit4();
    void resolve_manyConnected_matchingOutput();
};

void CliTest::parse_applyPerfect()
{
    const auto r = Cli::parse({QStringLiteral("kscaling"), QStringLiteral("--apply"), QStringLiteral("perfect")});
    QCOMPARE(r.exitCode, Cli::Success);
    QCOMPARE(r.command, Cli::Command::Apply);
    QCOMPARE(r.preset, QStringLiteral("perfect"));
    QVERIFY(r.output.isEmpty());
}

void CliTest::parse_applyMaxSpace()
{
    const auto r = Cli::parse({QStringLiteral("kscaling"), QStringLiteral("--apply"), QStringLiteral("max-space")});
    QCOMPARE(r.exitCode, Cli::Success);
    QCOMPARE(r.command, Cli::Command::Apply);
    QCOMPARE(r.preset, QStringLiteral("max-space"));
}

void CliTest::parse_applyComfort()
{
    const auto r = Cli::parse({QStringLiteral("kscaling"), QStringLiteral("--apply"), QStringLiteral("comfort")});
    QCOMPARE(r.exitCode, Cli::Success);
    QCOMPARE(r.command, Cli::Command::Apply);
    QCOMPARE(r.preset, QStringLiteral("comfort"));
}

void CliTest::parse_applyLarge()
{
    const auto r = Cli::parse({QStringLiteral("kscaling"), QStringLiteral("--apply"), QStringLiteral("large")});
    QCOMPARE(r.exitCode, Cli::Success);
    QCOMPARE(r.command, Cli::Command::Apply);
    QCOMPARE(r.preset, QStringLiteral("large"));
}

void CliTest::parse_applyWithOutput()
{
    const auto r = Cli::parse({QStringLiteral("kscaling"),
                               QStringLiteral("--apply"),
                               QStringLiteral("perfect"),
                               QStringLiteral("--output"),
                               QStringLiteral("DP-3")});
    QCOMPARE(r.exitCode, Cli::Success);
    QCOMPARE(r.command, Cli::Command::Apply);
    QCOMPARE(r.preset, QStringLiteral("perfect"));
    QCOMPARE(r.output, QStringLiteral("DP-3"));
}

void CliTest::parse_outputBeforeApply()
{
    const auto r = Cli::parse({QStringLiteral("kscaling"),
                               QStringLiteral("--output"),
                               QStringLiteral("DP-3"),
                               QStringLiteral("--apply"),
                               QStringLiteral("perfect")});
    QCOMPARE(r.exitCode, Cli::Success);
    QCOMPARE(r.command, Cli::Command::Apply);
    QCOMPARE(r.preset, QStringLiteral("perfect"));
    QCOMPARE(r.output, QStringLiteral("DP-3"));
}

void CliTest::parse_revert()
{
    const auto r = Cli::parse({QStringLiteral("kscaling"), QStringLiteral("--revert")});
    QCOMPARE(r.exitCode, Cli::Success);
    QCOMPARE(r.command, Cli::Command::Revert);
    QVERIFY(r.preset.isEmpty());
    QVERIFY(r.output.isEmpty());
}

void CliTest::parse_revertWithOutput()
{
    const auto r = Cli::parse({QStringLiteral("kscaling"),
                               QStringLiteral("--revert"),
                               QStringLiteral("--output"),
                               QStringLiteral("HDMI-A-1")});
    QCOMPARE(r.exitCode, Cli::Success);
    QCOMPARE(r.command, Cli::Command::Revert);
    QCOMPARE(r.output, QStringLiteral("HDMI-A-1"));
}

void CliTest::parse_list()
{
    const auto r = Cli::parse({QStringLiteral("kscaling"), QStringLiteral("--list")});
    QCOMPARE(r.exitCode, Cli::Success);
    QCOMPARE(r.command, Cli::Command::List);
}

void CliTest::parse_applySaved()
{
    const auto r = Cli::parse({QStringLiteral("kscaling"), QStringLiteral("--apply-saved")});
    QCOMPARE(r.exitCode, Cli::Success);
    QCOMPARE(r.command, Cli::Command::ApplySaved);
}

void CliTest::parse_help()
{
    const auto r = Cli::parse({QStringLiteral("kscaling"), QStringLiteral("--help")});
    QCOMPARE(r.exitCode, Cli::Success);
    QCOMPARE(r.command, Cli::Command::Help);
}

void CliTest::parse_unknownPreset_isUsageError()
{
    const auto r = Cli::parse({QStringLiteral("kscaling"), QStringLiteral("--apply"), QStringLiteral("tiny")});
    QCOMPARE(r.exitCode, Cli::UsageError);
    QCOMPARE(r.command, Cli::Command::Apply);
}

void CliTest::parse_applyMissingPreset_isUsageError()
{
    const auto r = Cli::parse({QStringLiteral("kscaling"), QStringLiteral("--apply")});
    QCOMPARE(r.exitCode, Cli::UsageError);
}

void CliTest::parse_outputWithList_isUsageError()
{
    const auto r = Cli::parse({QStringLiteral("kscaling"),
                               QStringLiteral("--list"),
                               QStringLiteral("--output"),
                               QStringLiteral("DP-3")});
    QCOMPARE(r.exitCode, Cli::UsageError);
    QCOMPARE(r.command, Cli::Command::List);
}

void CliTest::parse_outputWithApplySaved_isUsageError()
{
    const auto r = Cli::parse({QStringLiteral("kscaling"),
                               QStringLiteral("--apply-saved"),
                               QStringLiteral("--output"),
                               QStringLiteral("DP-3")});
    QCOMPARE(r.exitCode, Cli::UsageError);
    QCOMPARE(r.command, Cli::Command::ApplySaved);
}

void CliTest::parse_applyAndList_isUsageError()
{
    const auto r = Cli::parse({QStringLiteral("kscaling"),
                               QStringLiteral("--apply"),
                               QStringLiteral("perfect"),
                               QStringLiteral("--list")});
    QCOMPARE(r.exitCode, Cli::UsageError);
}

void CliTest::parse_unknownFlag_isUsageError()
{
    const auto r = Cli::parse({QStringLiteral("kscaling"), QStringLiteral("--doctor")});
    QCOMPARE(r.exitCode, Cli::UsageError);
}

void CliTest::formatApplySaved_empty_isNothingApplied()
{
    QCOMPARE(Cli::formatApplySaved({}), QStringLiteral("nothing applied\n"));
}

void CliTest::formatApplySaved_printsConnectorPresetCanvasScale()
{
    Cli::AppliedOutput applied;
    applied.connector = QStringLiteral("DP-3");
    applied.preset = QStringLiteral("perfect");
    applied.canvasW = 4096;
    applied.canvasH = 2304;
    applied.hz = 120;
    applied.scale = 2.00;
    QCOMPARE(Cli::formatApplySaved({applied}),
             QStringLiteral("DP-3  perfect  4096x2304 @ 120  scale 2.00\n"));
}

void CliTest::resolve_zeroConnected_isExit3()
{
    Cli::ParseResult parsed;
    parsed.exitCode = Cli::Success;
    parsed.command = Cli::Command::Apply;
    parsed.preset = QStringLiteral("perfect");
    const auto r = Cli::resolveOutput(parsed, {});
    QCOMPARE(r.exitCode, Cli::NoConnectedOutputs);
}

void CliTest::resolve_oneConnected_omittedOutput()
{
    Cli::ParseResult parsed;
    parsed.exitCode = Cli::Success;
    parsed.command = Cli::Command::Apply;
    parsed.preset = QStringLiteral("perfect");
    const auto r = Cli::resolveOutput(parsed, {QStringLiteral("DP-3")});
    QCOMPARE(r.exitCode, Cli::Success);
    QCOMPARE(r.output, QStringLiteral("DP-3"));
}

void CliTest::resolve_oneConnected_matchingOutput()
{
    Cli::ParseResult parsed;
    parsed.exitCode = Cli::Success;
    parsed.command = Cli::Command::Apply;
    parsed.preset = QStringLiteral("perfect");
    parsed.output = QStringLiteral("DP-3");
    const auto r = Cli::resolveOutput(parsed, {QStringLiteral("DP-3")});
    QCOMPARE(r.exitCode, Cli::Success);
    QCOMPARE(r.output, QStringLiteral("DP-3"));
}

void CliTest::resolve_oneConnected_mismatch_isExit4()
{
    Cli::ParseResult parsed;
    parsed.exitCode = Cli::Success;
    parsed.command = Cli::Command::Apply;
    parsed.output = QStringLiteral("HDMI-A-1");
    const auto r = Cli::resolveOutput(parsed, {QStringLiteral("DP-3")});
    QCOMPARE(r.exitCode, Cli::OutputRequiredOrUnknown);
}

void CliTest::resolve_manyConnected_omittedOutput_isExit4()
{
    Cli::ParseResult parsed;
    parsed.exitCode = Cli::Success;
    parsed.command = Cli::Command::Apply;
    parsed.preset = QStringLiteral("perfect");
    const auto r = Cli::resolveOutput(parsed, {QStringLiteral("DP-3"), QStringLiteral("HDMI-A-1")});
    QCOMPARE(r.exitCode, Cli::OutputRequiredOrUnknown);
    QVERIFY(r.error.contains(QStringLiteral("DP-3")));
    QVERIFY(r.error.contains(QStringLiteral("HDMI-A-1")));
}

void CliTest::resolve_manyConnected_unknown_isExit4()
{
    Cli::ParseResult parsed;
    parsed.exitCode = Cli::Success;
    parsed.command = Cli::Command::Revert;
    parsed.output = QStringLiteral("eDP-1");
    const auto r = Cli::resolveOutput(parsed, {QStringLiteral("DP-3"), QStringLiteral("HDMI-A-1")});
    QCOMPARE(r.exitCode, Cli::OutputRequiredOrUnknown);
}

void CliTest::resolve_manyConnected_matchingOutput()
{
    Cli::ParseResult parsed;
    parsed.exitCode = Cli::Success;
    parsed.command = Cli::Command::Revert;
    parsed.output = QStringLiteral("HDMI-A-1");
    const auto r = Cli::resolveOutput(parsed, {QStringLiteral("DP-3"), QStringLiteral("HDMI-A-1")});
    QCOMPARE(r.exitCode, Cli::Success);
    QCOMPARE(r.output, QStringLiteral("HDMI-A-1"));
}

QTEST_APPLESS_MAIN(CliTest)
#include "CliTest.moc"
