// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#include "ApplyService.h"

#include "Preset.h"
#include "ResolutionMath.h"
#include "Settings.h"

ApplyService::ApplyService(DisplayBackend& backend)
    : m_backend(backend)
{
}

ApplyService::Result ApplyService::apply(const QString& presetId, const ConnectorName& name)
{
    Result result;

    const std::optional<Preset> preset = findPreset(presetId);
    if (!preset) {
        result.exitCode = 1;
        return result;
    }

    const ListResult listed = m_backend.list();
    if (!listed.ok) {
        result.exitCode = 1;
        return result;
    }

    const OutputSnapshot* snapshot = nullptr;
    for (const OutputSnapshot& candidate : listed.snapshots) {
        if (candidate.name == name) {
            snapshot = &candidate;
            break;
        }
    }
    if (!snapshot || !snapshot->connected) {
        result.exitCode = 1;
        return result;
    }
    if (!snapshot->customModesCapable) {
        result.exitCode = 1;
        return result;
    }

    const ModePlan plan = ResolutionMath::plan(
        QSize(snapshot->native.size.w, snapshot->native.size.h),
        preset->uiScale);
    const qreal scaleBeforeAttempt = snapshot->scale;

    const ::Result applied = m_backend.applyCustom(
        name,
        Size{plan.canvas.width(), plan.canvas.height()},
        plan.hz,
        2.00);
    if (!applied.ok) {
        result.exitCode = 2;
        return result;
    }

    Settings::ProfileMap outputs = Settings::load().outputs;
    SavedProfile profile;
    if (outputs.contains(name)) {
        profile = outputs.value(name);
    } else {
        profile.originalScale = scaleBeforeAttempt;
    }
    profile.preset = presetId;
    profile.mode = plan.canvas;
    profile.hz = plan.hz;
    outputs.insert(name, profile);

    if (!Settings::save(outputs).ok) {
        result.exitCode = 1;
        return result;
    }

    result.ok = true;
    result.exitCode = 0;
    return result;
}
