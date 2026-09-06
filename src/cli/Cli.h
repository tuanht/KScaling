// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#pragma once

#include <QString>
#include <QStringList>

class Cli
{
public:
    enum ExitCode : int {
        Success = 0,
        UsageError = 1,
        CompositorRejected = 2,
        NoConnectedOutputs = 3,
        OutputRequiredOrUnknown = 4,
    };

    enum class Command {
        None,
        List,
        Apply,
        Revert,
        ApplySaved,
        Help,
    };

    struct ParseResult
    {
        ExitCode exitCode = UsageError;
        Command command = Command::None;
        QString preset;
        QString output;
        QString error;
    };

    struct ResolveResult
    {
        ExitCode exitCode = Success;
        QString output;
        QString error;
    };

    static ParseResult parse(const QStringList& args);
    static ResolveResult resolveOutput(const ParseResult& parsed, const QStringList& connectedNames);
};
