// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#include "Cli.h"

#include "Preset.h"
#include "ResolutionMath.h"

#include <QLatin1String>
#include <QSize>

namespace {

Cli::ParseResult usageError(Cli::Command command, const QString& error, const QString& preset = {},
                            const QString& output = {})
{
    Cli::ParseResult result;
    result.exitCode = Cli::UsageError;
    result.command = command;
    result.preset = preset;
    result.output = output;
    result.error = error;
    return result;
}

bool isCommandFlag(QStringView arg)
{
    return arg == QLatin1String("--apply") || arg == QLatin1String("--list") || arg == QLatin1String("--revert")
        || arg == QLatin1String("--apply-saved") || arg == QLatin1String("--help");
}

} // namespace

Cli::ParseResult Cli::parse(const QStringList& args)
{
    int i = 0;
    if (!args.isEmpty() && !args.first().startsWith(QLatin1Char('-'))) {
        i = 1;
    }

    Command command = Command::None;
    QString preset;
    QString output;

    while (i < args.size()) {
        const QString& arg = args.at(i);

        if (isCommandFlag(arg)) {
            if (command != Command::None) {
                return usageError(command, QStringLiteral("Flags are mutually exclusive"));
            }
            if (arg == QLatin1String("--apply")) {
                command = Command::Apply;
                if (i + 1 >= args.size() || args.at(i + 1).startsWith(QLatin1Char('-'))) {
                    return usageError(command, QStringLiteral("Missing preset"));
                }
                ++i;
                preset = args.at(i);
            } else if (arg == QLatin1String("--list")) {
                command = Command::List;
            } else if (arg == QLatin1String("--revert")) {
                command = Command::Revert;
            } else if (arg == QLatin1String("--apply-saved")) {
                command = Command::ApplySaved;
            } else {
                command = Command::Help;
            }
        } else if (arg == QLatin1String("--output")) {
            if (!output.isEmpty()) {
                return usageError(command, QStringLiteral("Flags are mutually exclusive"));
            }
            if (i + 1 >= args.size() || args.at(i + 1).startsWith(QLatin1Char('-'))) {
                return usageError(command, QStringLiteral("Missing output"));
            }
            ++i;
            output = args.at(i);
        } else {
            return usageError(command, QStringLiteral("Unknown argument: %1").arg(arg));
        }
        ++i;
    }

    if (command == Command::None) {
        return usageError(Command::None, QStringLiteral("Missing command"));
    }

    if (!output.isEmpty() && command != Command::Apply && command != Command::Revert) {
        return usageError(command, QStringLiteral("--output is only valid with --apply or --revert"), preset, output);
    }

    if (command == Command::Apply && !findPreset(preset)) {
        return usageError(command, QStringLiteral("Unknown preset: %1").arg(preset), preset, output);
    }

    ParseResult result;
    result.exitCode = Success;
    result.command = command;
    result.preset = preset;
    result.output = output;
    return result;
}

Cli::ResolveResult Cli::resolveOutput(const ParseResult& parsed, const QStringList& connectedNames)
{
    ResolveResult result;

    if (connectedNames.isEmpty()) {
        result.exitCode = NoConnectedOutputs;
        result.error = QStringLiteral("No connected outputs");
        return result;
    }

    if (parsed.output.isEmpty()) {
        if (connectedNames.size() == 1) {
            result.output = connectedNames.first();
            return result;
        }
        result.exitCode = OutputRequiredOrUnknown;
        result.error = QStringLiteral("Multiple outputs connected; pass --output <connector>. Connected: %1")
                           .arg(connectedNames.join(QLatin1String(", ")));
        return result;
    }

    if (!connectedNames.contains(parsed.output)) {
        result.exitCode = OutputRequiredOrUnknown;
        result.error = QStringLiteral("Unknown output '%1'. Connected: %2")
                           .arg(parsed.output, connectedNames.join(QLatin1String(", ")));
        return result;
    }

    result.output = parsed.output;
    return result;
}

namespace {

QString formatSize(Size size)
{
    return QStringLiteral("%1x%2").arg(size.w).arg(size.h);
}

QString formatHeader(const OutputSnapshot& snapshot)
{
    const QString capability = snapshot.customModesCapable ? QStringLiteral("capable")
                                                           : QStringLiteral("incapable");
    return QStringLiteral("%1  %2  native %3  current %4 @ %5  scale %6")
        .arg(snapshot.name, capability, formatSize(snapshot.native.size), formatSize(snapshot.current.size),
             QString::number(snapshot.current.hz, 'f', 2), QString::number(snapshot.scale, 'f', 2));
}

QString formatPresetRow(const Preset& preset, QSize native)
{
    const ModePlan plan = ResolutionMath::plan(native, preset.uiScale);
    const QString looks = QStringLiteral("%1x%2").arg(plan.looksLike.width()).arg(plan.looksLike.height());
    const QString canvas = QStringLiteral("%1x%2").arg(plan.canvas.width()).arg(plan.canvas.height());
    return QStringLiteral("  %1  looks %2  mode %3 @ %4")
        .arg(QLatin1String(preset.id), -9)
        .arg(looks, -9)
        .arg(canvas)
        .arg(qRound(plan.hz));
}

} // namespace

QString Cli::formatList(const ListResult& listed)
{
    QString text;
    for (const OutputSnapshot& snapshot : listed.snapshots) {
        if (!snapshot.connected) {
            continue;
        }
        text += formatHeader(snapshot);
        text += QLatin1Char('\n');
        if (!snapshot.customModesCapable) {
            continue;
        }
        const QSize native(snapshot.native.size.w, snapshot.native.size.h);
        for (const Preset& preset : kPresets) {
            text += formatPresetRow(preset, native);
            text += QLatin1Char('\n');
        }
    }
    return text;
}
