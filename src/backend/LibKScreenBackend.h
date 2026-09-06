// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#pragma once

#include "DisplayBackend.h"

class LibKScreenBackend : public DisplayBackend
{
public:
    ListResult list() override;

    Result applyCustom(const ConnectorName& name,
                       Size canvas,
                       qreal hz,
                       qreal scale = 2.00) override;

    Result revert(const ConnectorName& name,
                  std::optional<qreal> originalScale = std::nullopt) override;
};
