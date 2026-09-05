// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#pragma once

#include <QSize>
#include <QtGlobal>

struct ModePlan
{
    QSize looksLike;
    QSize canvas;
    qreal scale;
    qreal hz;
};

class ResolutionMath
{
public:
    static ModePlan plan(QSize native, qreal uiScale);
};
