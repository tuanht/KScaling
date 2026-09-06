// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#include "LibKScreenBackend.h"

#include <KScreen/Config>
#include <KScreen/GetConfigOperation>
#include <KScreen/Mode>
#include <KScreen/Output>
#include <KScreen/SetConfigOperation>

#include <QSize>
#include <QtGlobal>

namespace {

KScreen::ConfigPtr getConfig(QString* error)
{
    KScreen::GetConfigOperation op;
    op.exec();
    if (op.hasError() || !op.config()) {
        *error = op.errorString().isEmpty() ? QStringLiteral("GetConfig failed") : op.errorString();
        return {};
    }
    return op.config();
}

KScreen::OutputPtr findOutputByName(const KScreen::ConfigPtr& config, const ConnectorName& name)
{
    if (!config) {
        return {};
    }
    for (const KScreen::OutputPtr& output : config->outputs()) {
        if (output && output->name() == name) {
            return output;
        }
    }
    return {};
}

bool hasCustomMode(const KScreen::OutputPtr& output, Size canvas, qreal hz)
{
    const QSize size(canvas.w, canvas.h);
    for (const KScreen::ModeInfo& info : output->customModes()) {
        if (info.size == size && qRound(info.refreshRate) == qRound(hz)) {
            return true;
        }
    }
    return false;
}

KScreen::ModePtr pickMode(const KScreen::OutputPtr& output, Size canvas, qreal hz)
{
    KScreen::ModePtr best;
    qreal bestDelta = 0;
    const QSize size(canvas.w, canvas.h);
    for (const KScreen::ModePtr& mode : output->modes()) {
        if (!mode || mode->id().isEmpty() || mode->size() != size) {
            continue;
        }
        const qreal delta = qAbs(qreal(mode->refreshRate()) - hz);
        if (!best || delta < bestDelta) {
            best = mode;
            bestDelta = delta;
        }
    }
    return best;
}

Mode toMode(const KScreen::ModePtr& mode)
{
    Mode result;
    result.size = {mode->size().width(), mode->size().height()};
    result.hz = mode->refreshRate();
    result.id = mode->id();
    return result;
}

Result setConfig(const KScreen::ConfigPtr& config)
{
    KScreen::SetConfigOperation op(config);
    op.exec();
    if (op.hasError()) {
        return {false,
                op.errorString().isEmpty() ? QStringLiteral("SetConfig failed") : op.errorString()};
    }
    return {true, {}};
}

}

ListResult LibKScreenBackend::list()
{
    QString error;
    const KScreen::ConfigPtr config = getConfig(&error);
    if (!config) {
        return {false, error, {}};
    }

    ListResult result;
    result.ok = true;
    for (const KScreen::OutputPtr& output : config->outputs()) {
        if (!output || !output->isConnected()) {
            continue;
        }

        const KScreen::ModePtr preferred = output->preferredMode();
        const KScreen::ModePtr current = output->currentMode();
        if (!preferred || !current) {
            return {false,
                    QStringLiteral("output %1 missing preferred/current mode").arg(output->name()),
                    {}};
        }

        OutputSnapshot snapshot;
        snapshot.name = output->name();
        snapshot.connected = true;
        snapshot.customModesCapable
            = output->capabilities().testFlag(KScreen::Output::Capability::CustomModes);
        snapshot.native = toMode(preferred);
        snapshot.current = toMode(current);
        snapshot.scale = output->scale();
        for (const KScreen::ModeInfo& info : output->customModes()) {
            snapshot.customModes.append(
                SizeHz{{info.size.width(), info.size.height()}, info.refreshRate});
        }
        result.snapshots.append(snapshot);
    }
    return result;
}

Result LibKScreenBackend::applyCustom(const ConnectorName& name,
                                      Size canvas,
                                      qreal hz,
                                      qreal scale)
{
    Q_UNUSED(scale);

    QString error;
    KScreen::ConfigPtr config = getConfig(&error);
    if (!config) {
        return {false, error};
    }

    KScreen::OutputPtr output = findOutputByName(config, name);
    if (!output) {
        return {false, QStringLiteral("output not found")};
    }
    if (!output->isConnected()) {
        return {false, QStringLiteral("output disconnected")};
    }
    if (!output->capabilities().testFlag(KScreen::Output::Capability::CustomModes)) {
        return {false, QStringLiteral("CustomModes not supported")};
    }

    const qreal scaleBeforeAttempt = output->scale();

    if (!hasCustomMode(output, canvas, hz)) {
        QList<KScreen::ModeInfo> modes = output->customModes();
        modes.append(KScreen::ModeInfo{QSize(canvas.w, canvas.h),
                                       float(hz),
                                       KScreen::ModeInfo::Flag::Custom});
        output->setCustomModes(modes);
        const Result phaseA = setConfig(config);
        if (!phaseA.ok) {
            return phaseA;
        }
    }

    config = getConfig(&error);
    if (!config) {
        return {false, error};
    }
    output = findOutputByName(config, name);
    if (!output) {
        return {false, QStringLiteral("output not found")};
    }
    if (!output->isConnected()) {
        return {false, QStringLiteral("output disconnected")};
    }

    const KScreen::ModePtr picked = pickMode(output, canvas, hz);
    if (!picked || picked->id().isEmpty()) {
        return {false, QStringLiteral("mode not found")};
    }

    output->setCurrentModeId(picked->id());
    output->setScale(2.0);
    const Result phaseB = setConfig(config);
    if (!phaseB.ok) {
        output->setCurrentModeId(output->preferredModeId());
        output->setScale(scaleBeforeAttempt);
        setConfig(config);
        return phaseB;
    }
    return {true, {}};
}

Result LibKScreenBackend::revert(const ConnectorName& name, std::optional<qreal> originalScale)
{
    QString error;
    KScreen::ConfigPtr config = getConfig(&error);
    if (!config) {
        return {false, error};
    }

    KScreen::OutputPtr output = findOutputByName(config, name);
    if (!output) {
        return {false, QStringLiteral("output not found")};
    }
    if (!output->isConnected()) {
        return {false, QStringLiteral("output disconnected")};
    }

    output->setCurrentModeId(output->preferredModeId());
    if (originalScale.has_value()) {
        output->setScale(*originalScale);
    }
    return setConfig(config);
}
