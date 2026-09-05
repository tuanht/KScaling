// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#pragma once

#include <QLatin1String>
#include <QStringView>
#include <QtGlobal>

#include <array>
#include <optional>

struct Preset
{
    const char* id;
    qreal uiScale;
};

inline constexpr std::array<Preset, 4> kPresets{{
    {"perfect", 1.25},
    {"max-space", 1.00},
    {"comfort", 1.60},
    {"large", 1.778},
}};

inline std::optional<Preset> findPreset(QStringView id)
{
    for (const Preset& preset : kPresets) {
        if (id == QLatin1String(preset.id)) {
            return preset;
        }
    }
    return std::nullopt;
}
