// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#pragma once

#include <QList>
#include <QString>
#include <QtGlobal>

#include <optional>

// Identity is connector Output::name() (e.g. DP-3), never numeric KScreen id.
using ConnectorName = QString;

struct Size
{
    int w = 0;
    int h = 0;
};

struct Mode
{
    Size size;
    qreal hz = 0;
    // Compositor-local; valid only for the snapshot this Mode came from.
    QString id;
};

struct SizeHz
{
    Size size;
    qreal hz = 0;
};

struct OutputSnapshot
{
    ConnectorName name;
    bool connected = false;
    bool customModesCapable = false;
    Mode native;
    Mode current;
    qreal scale = 1.0;
    QList<SizeHz> customModes;
};

struct Result
{
    bool ok = false;
    QString error;
};

struct ListResult
{
    bool ok = false;
    QString error;
    QList<OutputSnapshot> snapshots;
};

class DisplayBackend
{
public:
    virtual ~DisplayBackend() = default;

    // Fresh compositor read. No kscreen-doctor / QProcess.
    virtual ListResult list() = 0;

    // Two-phase apply (libkscreen GetConfig / SetConfig). Interface only.
    // 1. GetConfig. Find name. Error if missing/disconnected.
    // 2. Error if !customModesCapable.
    // 3. Native = preferred mode of that snapshot.
    // 4. If no custom mode with same size and qRound(hz): append Custom
    //    full-blanking ModeInfo to the full list, SetConfig (phase A).
    //    On error: return reject; do not switch.
    // 5. GetConfig again (new snapshot; do not reuse ids from step 1).
    // 6. Pick mode: size match, nearest refreshRate. Error if none.
    // 7. setCurrentModeId(that id); setScale(2.00); SetConfig (phase B).
    // 8. On error: setCurrentModeId(preferredModeId); setScale(scaleBeforeAttempt);
    //    SetConfig; return reject with compositor error text.
    // 9. Success: caller persists SavedProfile.
    virtual Result applyCustom(const ConnectorName& name,
                               Size canvas,
                               qreal hz,
                               qreal scale = 2.00)
        = 0;

    // GetConfig, setCurrentModeId(preferredModeId), if originalScale present
    // setScale(originalScale), SetConfig. Do not clear SavedProfile.
    virtual Result revert(const ConnectorName& name,
                          std::optional<qreal> originalScale = std::nullopt)
        = 0;
};
