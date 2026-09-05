// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#include "Settings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QStandardPaths>

namespace {

bool looksLikeConnectorName(const QString& key)
{
    if (key.isEmpty()) {
        return false;
    }
    bool isInteger = false;
    key.toInt(&isInteger);
    return !isInteger;
}

SavedProfile profileFromJson(const QJsonObject& object)
{
    SavedProfile profile;
    profile.preset = object.value(QStringLiteral("preset")).toString();
    const QJsonArray mode = object.value(QStringLiteral("mode")).toArray();
    if (mode.size() >= 2) {
        profile.mode = QSize(mode.at(0).toInt(), mode.at(1).toInt());
    }
    profile.hz = object.value(QStringLiteral("hz")).toDouble();
    profile.originalScale = object.value(QStringLiteral("originalScale")).toDouble();
    return profile;
}

QJsonObject profileToJson(const SavedProfile& profile)
{
    QJsonArray mode;
    mode.append(profile.mode.width());
    mode.append(profile.mode.height());

    QJsonObject object;
    object.insert(QStringLiteral("preset"), profile.preset);
    object.insert(QStringLiteral("mode"), mode);
    object.insert(QStringLiteral("hz"), profile.hz);
    object.insert(QStringLiteral("originalScale"), profile.originalScale);
    return object;
}

}

QString Settings::filePath()
{
    if (QCoreApplication::applicationName().isEmpty()) {
        QCoreApplication::setApplicationName(QStringLiteral("kscaling"));
    }
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation))
        .filePath(QStringLiteral("profiles.json"));
}

Settings::LoadResult Settings::load()
{
    LoadResult result;
    QFile file(filePath());
    if (!file.exists()) {
        result.ok = true;
        return result;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        result.error = file.errorString();
        return result;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        result.error = QStringLiteral("invalid profiles.json");
        return result;
    }

    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("version")).toInt(-1) != 1) {
        result.error = QStringLiteral("unknown profiles.json version");
        return result;
    }

    const QJsonValue outputsValue = root.value(QStringLiteral("outputs"));
    if (outputsValue.isUndefined()) {
        result.ok = true;
        return result;
    }
    if (!outputsValue.isObject()) {
        result.error = QStringLiteral("outputs is not an object");
        return result;
    }

    const QJsonObject outputs = outputsValue.toObject();
    for (auto it = outputs.begin(); it != outputs.end(); ++it) {
        if (!looksLikeConnectorName(it.key())) {
            result.outputs.clear();
            result.error = QStringLiteral("connector key is not a connector name");
            return result;
        }
        if (!it.value().isObject()) {
            result.outputs.clear();
            result.error = QStringLiteral("profile is not an object");
            return result;
        }
        result.outputs.insert(it.key(), profileFromJson(it.value().toObject()));
    }

    result.ok = true;
    return result;
}

Settings::SaveResult Settings::save(const ProfileMap& outputs)
{
    SaveResult result;
    QJsonObject outputsObject;
    for (auto it = outputs.begin(); it != outputs.end(); ++it) {
        if (!looksLikeConnectorName(it.key())) {
            result.error = QStringLiteral("connector key is not a connector name");
            return result;
        }
        outputsObject.insert(it.key(), profileToJson(it.value()));
    }

    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("outputs"), outputsObject);

    const QString path = filePath();
    if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
        result.error = QStringLiteral("could not create config directory");
        return result;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        result.error = file.errorString();
        return result;
    }
    const QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(bytes) != bytes.size()) {
        result.error = file.errorString();
        return result;
    }

    result.ok = true;
    return result;
}
