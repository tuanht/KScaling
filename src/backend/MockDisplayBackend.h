// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#pragma once

#include "DisplayBackend.h"

#include <QHash>

class MockDisplayBackend : public DisplayBackend
{
public:
    ListResult list() override;

    Result applyCustom(const ConnectorName& name,
                       Size canvas,
                       qreal hz,
                       qreal scale = 2.00) override;

    Result revert(const ConnectorName& name,
                  std::optional<qreal> originalScale = std::nullopt) override;

    void injectOutput(const OutputSnapshot& snapshot);
    void setFailPhaseA(bool fail, const QString& error = {});
    void setFailPhaseB(bool fail, const QString& error = {});
    void simulateReload();

    Result setCurrentModeId(const ConnectorName& name, const QString& modeId);

private:
    struct OutputState
    {
        ConnectorName name;
        bool connected = false;
        bool customModesCapable = false;
        qreal scale = 1.0;
        QString preferredModeId;
        QString currentModeId;
        QList<Mode> modes;
        QList<SizeHz> customModes;
    };

    OutputState* findOutput(const ConnectorName& name);
    OutputSnapshot toSnapshot(const OutputState& output) const;
    const Mode* findMode(const OutputState& output, const QString& id) const;
    const Mode* pickMode(const OutputState& output, Size canvas, qreal hz) const;
    bool hasCustomMode(const OutputState& output, Size canvas, qreal hz) const;
    QString allocateModeId();
    void noteId(const QString& id);
    void rebuildModesForCurrentGeneration(OutputState& output);

    QList<OutputState> m_outputs;
    int m_snapshotGeneration = 0;
    int m_nextModeId = 1;
    QHash<QString, int> m_idGeneration;
    bool m_failPhaseA = false;
    bool m_failPhaseB = false;
    QString m_phaseAError;
    QString m_phaseBError;
};
