// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#include "ApplyService.h"

#include "Preset.h"
#include "ResolutionMath.h"
#include "Settings.h"

#include <optional>

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

ApplyService::Result ApplyService::applySaved()
{
    Result result;

    const Settings::LoadResult loaded = Settings::load();
    if (!loaded.ok) {
        result.exitCode = 1;
        result.error = loaded.error;
        return result;
    }
    if (loaded.outputs.isEmpty()) {
        result.ok = true;
        result.exitCode = 0;
        return result;
    }

    const ListResult listed = m_backend.list();
    if (!listed.ok) {
        result.exitCode = 1;
        return result;
    }

    Settings::ProfileMap outputs = loaded.outputs;

    for (auto it = loaded.outputs.cbegin(); it != loaded.outputs.cend(); ++it) {
        const ConnectorName& name = it.key();
        SavedProfile profile = it.value();

        const OutputSnapshot* snapshot = nullptr;
        for (const OutputSnapshot& candidate : listed.snapshots) {
            if (candidate.name == name) {
                snapshot = &candidate;
                break;
            }
        }
        if (!snapshot || !snapshot->connected) {
            continue;
        }
        if (!snapshot->customModesCapable) {
            result.exitCode = 1;
            return result;
        }

        Size canvas;
        qreal hz = 0;
        if (profile.mode.isValid() && !profile.mode.isEmpty() && profile.hz > 0) {
            canvas = Size{profile.mode.width(), profile.mode.height()};
            hz = profile.hz;
        } else {
            const std::optional<Preset> preset = findPreset(profile.preset);
            if (!preset) {
                result.exitCode = 1;
                return result;
            }
            const ModePlan plan = ResolutionMath::plan(
                QSize(snapshot->native.size.w, snapshot->native.size.h),
                preset->uiScale);
            canvas = Size{plan.canvas.width(), plan.canvas.height()};
            hz = plan.hz;
        }

        const qreal scaleBeforeAttempt = snapshot->scale;
        const ::Result applied = m_backend.applyCustom(name, canvas, hz, 2.00);
        if (!applied.ok) {
            result.exitCode = 2;
            return result;
        }

        if (!outputs.contains(name)) {
            profile.originalScale = scaleBeforeAttempt;
        }
        profile.mode = QSize(canvas.w, canvas.h);
        profile.hz = hz;
        outputs.insert(name, profile);

        if (!Settings::save(outputs).ok) {
            result.exitCode = 1;
            return result;
        }

        Applied recorded;
        recorded.connector = name;
        recorded.preset = profile.preset;
        recorded.canvas = canvas;
        recorded.hz = hz;
        recorded.scale = 2.00;
        result.applied.append(recorded);
    }

    result.ok = true;
    result.exitCode = 0;
    return result;
}

ApplyService::Result ApplyService::revert(const ConnectorName& name)
{
    Result result;

    std::optional<qreal> originalScale;
    const Settings::LoadResult loaded = Settings::load();
    if (loaded.ok && loaded.outputs.contains(name)) {
        originalScale = loaded.outputs.value(name).originalScale;
    }

    const ::Result reverted = m_backend.revert(name, originalScale);
    if (!reverted.ok) {
        result.exitCode = 1;
        result.error = reverted.error;
        return result;
    }

    result.ok = true;
    result.exitCode = 0;
    if (!originalScale.has_value()) {
        result.error = QStringLiteral("no saved prior scale");
    }
    return result;
}
