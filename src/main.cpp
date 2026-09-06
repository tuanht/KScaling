// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#include "backend/LibKScreenBackend.h"
#include "cli/Cli.h"
#include "core/ApplyService.h"

#include <QGuiApplication>
#include <QStringList>
#include <QTextStream>

int main(int argc, char* argv[])
{
    QGuiApplication::setDesktopSettingsAware(false);
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("kscaling"));

    const Cli::ParseResult parsed = Cli::parse(QGuiApplication::arguments());
    if (parsed.exitCode != Cli::Success) {
        if (!parsed.error.isEmpty()) {
            QTextStream(stderr) << parsed.error << '\n';
        }
        return parsed.exitCode;
    }

    if (parsed.command == Cli::Command::Help) {
        QTextStream(stdout) << Cli::formatHelp();
        return Cli::Success;
    }

    LibKScreenBackend backend;
    if (parsed.command == Cli::Command::ApplySaved) {
        ApplyService service(backend);
        const ApplyService::Result restored = service.applySaved();
        if (!restored.error.isEmpty()) {
            QTextStream(stderr) << restored.error << '\n';
        }
        if (restored.ok) {
            QList<Cli::AppliedOutput> lines;
            for (const ApplyService::Applied& item : restored.applied) {
                Cli::AppliedOutput line;
                line.connector = item.connector;
                line.preset = item.preset;
                line.canvasW = item.canvas.w;
                line.canvasH = item.canvas.h;
                line.hz = item.hz;
                line.scale = item.scale;
                lines.append(line);
            }
            QTextStream(stdout) << Cli::formatApplySaved(lines);
        }
        return restored.exitCode;
    }

    const ListResult listed = backend.list();
    if (parsed.command == Cli::Command::List) {
        if (!listed.ok) {
            if (!listed.error.isEmpty()) {
                QTextStream(stderr) << listed.error << '\n';
            }
            return Cli::UsageError;
        }
        QTextStream(stdout) << Cli::formatList(listed);
        return Cli::Success;
    }

    if (parsed.command != Cli::Command::Apply && parsed.command != Cli::Command::Revert) {
        return Cli::UsageError;
    }
    if (!listed.ok) {
        if (!listed.error.isEmpty()) {
            QTextStream(stderr) << listed.error << '\n';
        }
        return Cli::UsageError;
    }

    QStringList connected;
    for (const OutputSnapshot& snapshot : listed.snapshots) {
        if (snapshot.connected) {
            connected.append(snapshot.name);
        }
    }

    const Cli::ResolveResult resolved = Cli::resolveOutput(parsed, connected);
    if (resolved.exitCode != Cli::Success) {
        if (!resolved.error.isEmpty()) {
            QTextStream(stderr) << resolved.error << '\n';
        }
        return resolved.exitCode;
    }

    ApplyService service(backend);
    if (parsed.command == Cli::Command::Revert) {
        const ApplyService::Result reverted = service.revert(resolved.output);
        if (!reverted.error.isEmpty()) {
            QTextStream(stderr) << reverted.error << '\n';
        }
        return reverted.exitCode;
    }

    return service.apply(parsed.preset, resolved.output).exitCode;
}
