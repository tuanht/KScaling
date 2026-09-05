// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#pragma once

#include "Profile.h"

#include <QMap>
#include <QString>

class Settings
{
public:
    using ProfileMap = QMap<QString, SavedProfile>;

    struct LoadResult
    {
        bool ok = false;
        QString error;
        ProfileMap outputs;
    };

    struct SaveResult
    {
        bool ok = false;
        QString error;
    };

    static QString filePath();
    static LoadResult load();
    static SaveResult save(const ProfileMap& outputs);
};
