// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#include "MockDisplayBackend.h"

#include <QtGlobal>

namespace {

bool sameSize(Size a, Size b)
{
    return a.w == b.w && a.h == b.h;
}

bool sameSizeHz(Size a, qreal aHz, Size b, qreal bHz)
{
    return sameSize(a, b) && qRound(aHz) == qRound(bHz);
}

}

ListResult MockDisplayBackend::list()
{
    ListResult result;
    result.ok = true;
    for (const OutputState& output : m_outputs) {
        result.snapshots.append(toSnapshot(output));
    }
    return result;
}

Result MockDisplayBackend::applyCustom(const ConnectorName& name,
                                       Size canvas,
                                       qreal hz,
                                       qreal scale)
{
    OutputState* output = findOutput(name);
    if (!output) {
        return {false, QStringLiteral("output not found")};
    }
    if (!output->connected) {
        return {false, QStringLiteral("output disconnected")};
    }
    if (!output->customModesCapable) {
        return {false, QStringLiteral("CustomModes not supported")};
    }

    const qreal scaleBeforeAttempt = output->scale;
    const QString preferredModeId = output->preferredModeId;

    if (!hasCustomMode(*output, canvas, hz)) {
        if (m_failPhaseA) {
            const QString error = m_phaseAError.isEmpty() ? QStringLiteral("phase A failed")
                                                          : m_phaseAError;
            return {false, error};
        }
        output->customModes.append(SizeHz{canvas, hz});
        registerCustomModes(*output);
    }

    const Mode* picked = pickMode(*output, canvas, hz);
    if (!picked || picked->id.isEmpty()) {
        return {false, QStringLiteral("mode not found")};
    }

    if (m_failPhaseB) {
        output->currentModeId = preferredModeId;
        output->scale = scaleBeforeAttempt;
        const QString error = m_phaseBError.isEmpty() ? QStringLiteral("phase B failed")
                                                      : m_phaseBError;
        return {false, error};
    }

    const Result switched = setCurrentModeId(name, picked->id);
    if (!switched.ok) {
        output->currentModeId = preferredModeId;
        output->scale = scaleBeforeAttempt;
        return switched;
    }
    output->scale = scale;
    return {true, {}};
}

Result MockDisplayBackend::revert(const ConnectorName& name, std::optional<qreal> originalScale)
{
    OutputState* output = findOutput(name);
    if (!output) {
        return {false, QStringLiteral("output not found")};
    }
    if (!output->connected) {
        return {false, QStringLiteral("output disconnected")};
    }

    const Result switched = setCurrentModeId(name, output->preferredModeId);
    if (!switched.ok) {
        return switched;
    }
    if (originalScale.has_value()) {
        output->scale = originalScale.value();
    }
    return {true, {}};
}

void MockDisplayBackend::injectOutput(const OutputSnapshot& snapshot)
{
    OutputState state;
    state.name = snapshot.name;
    state.connected = snapshot.connected;
    state.customModesCapable = snapshot.customModesCapable;
    state.scale = snapshot.scale;
    state.customModes = snapshot.customModes;

    Mode native = snapshot.native;
    if (native.id.isEmpty()) {
        native.id = allocateModeId();
    } else {
        noteId(native.id);
    }
    state.modes.append(native);
    state.preferredModeId = native.id;

    Mode current = snapshot.current;
    if (sameSizeHz(current.size, current.hz, native.size, native.hz)) {
        state.currentModeId = native.id;
    } else {
        if (current.id.isEmpty() || current.id == native.id) {
            current.id = allocateModeId();
        } else {
            noteId(current.id);
        }
        state.modes.append(current);
        state.currentModeId = current.id;
    }

    registerCustomModes(state);

    for (int i = 0; i < m_outputs.size(); ++i) {
        if (m_outputs.at(i).name == snapshot.name) {
            m_outputs[i] = state;
            return;
        }
    }
    m_outputs.append(state);
}

void MockDisplayBackend::setFailPhaseA(bool fail, const QString& error)
{
    m_failPhaseA = fail;
    m_phaseAError = error;
}

void MockDisplayBackend::setFailPhaseB(bool fail, const QString& error)
{
    m_failPhaseB = fail;
    m_phaseBError = error;
}

Result MockDisplayBackend::setCurrentModeId(const ConnectorName& name, const QString& modeId)
{
    OutputState* output = findOutput(name);
    if (!output) {
        return {false, QStringLiteral("output not found")};
    }
    if (modeId.isEmpty() || !findMode(*output, modeId)) {
        return {false, QStringLiteral("invalid mode id")};
    }
    output->currentModeId = modeId;
    return {true, {}};
}

MockDisplayBackend::OutputState* MockDisplayBackend::findOutput(const ConnectorName& name)
{
    for (OutputState& output : m_outputs) {
        if (output.name == name) {
            return &output;
        }
    }
    return nullptr;
}

OutputSnapshot MockDisplayBackend::toSnapshot(const OutputState& output) const
{
    OutputSnapshot snapshot;
    snapshot.name = output.name;
    snapshot.connected = output.connected;
    snapshot.customModesCapable = output.customModesCapable;
    snapshot.scale = output.scale;
    snapshot.customModes = output.customModes;
    if (const Mode* native = findMode(output, output.preferredModeId)) {
        snapshot.native = *native;
    }
    if (const Mode* current = findMode(output, output.currentModeId)) {
        snapshot.current = *current;
    }
    return snapshot;
}

const Mode* MockDisplayBackend::findMode(const OutputState& output, const QString& id) const
{
    if (id.isEmpty()) {
        return nullptr;
    }
    for (const Mode& mode : output.modes) {
        if (mode.id == id) {
            return &mode;
        }
    }
    return nullptr;
}

const Mode* MockDisplayBackend::pickMode(const OutputState& output, Size canvas, qreal hz) const
{
    const Mode* best = nullptr;
    qreal bestDelta = 0;
    for (const Mode& mode : output.modes) {
        if (mode.id.isEmpty() || !sameSize(mode.size, canvas)) {
            continue;
        }
        const qreal delta = qAbs(mode.hz - hz);
        if (!best || delta < bestDelta) {
            best = &mode;
            bestDelta = delta;
        }
    }
    return best;
}

bool MockDisplayBackend::hasCustomMode(const OutputState& output, Size canvas, qreal hz) const
{
    for (const SizeHz& mode : output.customModes) {
        if (sameSizeHz(mode.size, mode.hz, canvas, hz)) {
            return true;
        }
    }
    return false;
}

QString MockDisplayBackend::addSwitchableMode(OutputState& output, Size size, qreal hz)
{
    for (const Mode& mode : output.modes) {
        if (sameSizeHz(mode.size, mode.hz, size, hz) && !mode.id.isEmpty()) {
            return mode.id;
        }
    }
    Mode mode;
    mode.size = size;
    mode.hz = hz;
    mode.id = allocateModeId();
    output.modes.append(mode);
    return mode.id;
}

QString MockDisplayBackend::allocateModeId()
{
    return QString::number(m_nextModeId++);
}

void MockDisplayBackend::noteId(const QString& id)
{
    bool ok = false;
    const int value = id.toInt(&ok);
    if (ok && value >= m_nextModeId) {
        m_nextModeId = value + 1;
    }
}

void MockDisplayBackend::registerCustomModes(OutputState& output)
{
    for (const SizeHz& custom : output.customModes) {
        addSwitchableMode(output, custom.size, custom.hz);
    }
}
