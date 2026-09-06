// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#include "backend/LibKScreenBackend.h"
#include "cli/Cli.h"
#include "core/ApplyService.h"

#include <QCoreApplication>
#include <QStringList>
#include <QTextStream>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("kscaling"));

    const Cli::ParseResult parsed = Cli::parse(QCoreApplication::arguments());
    if (parsed.exitCode != Cli::Success) {
        if (!parsed.error.isEmpty()) {
            QTextStream(stderr) << parsed.error << '\n';
        }
        return parsed.exitCode;
    }

    LibKScreenBackend backend;
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
