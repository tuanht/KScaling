// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#pragma once

#include <QSize>
#include <QString>
#include <QtGlobal>

struct SavedProfile
{
    QString preset;
    QSize mode;
    qreal hz = 0;
    qreal originalScale = 0;
};
