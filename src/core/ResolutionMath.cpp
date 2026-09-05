// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#include "ResolutionMath.h"

namespace {

int roundNearestMultiple8(qreal value)
{
    return qRound(value / 8.0) * 8;
}

}

ModePlan ResolutionMath::plan(const QSize native, const qreal uiScale)
{
    const int looksLikeW = roundNearestMultiple8(qreal(native.width()) / uiScale);
    const int looksLikeH = qRound(qreal(looksLikeW) * native.height() / native.width());
    const QSize looksLike(looksLikeW, looksLikeH);
    const QSize canvas(looksLikeW * 2, looksLikeH * 2);
    const qreal hz = (canvas.width() >= 2 * native.width()) ? 60.0 : 120.0;
    return {looksLike, canvas, 2.0, hz};
}
