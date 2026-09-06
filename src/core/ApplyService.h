// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#pragma once

#include "DisplayBackend.h"

#include <QString>

class ApplyService
{
public:
    struct Result
    {
        bool ok = false;
        int exitCode = 1;
        QString error;
    };

    explicit ApplyService(DisplayBackend& backend);

    Result apply(const QString& presetId, const ConnectorName& name);
    Result applySaved();
    Result revert(const ConnectorName& name);

private:
    DisplayBackend& m_backend;
};
