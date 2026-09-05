// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#include "Settings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTest>

class SettingsTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void filePath_isAppConfigLocationProfilesJson();
    void load_missingFile_isEmpty();
    void load_missingOutputs_isEmpty();
    void load_unknownVersion_isError();
    void load_integerConnectorKey_isError();
    void save_roundtrip_dp3Profile();
    void save_integerConnectorKey_isError();
    void save_doesNotWriteR3Keys();
};

void SettingsTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setApplicationName(QStringLiteral("kscaling"));
}

void SettingsTest::cleanup()
{
    QFile::remove(Settings::filePath());
}

void SettingsTest::filePath_isAppConfigLocationProfilesJson()
{
    const QString path = Settings::filePath();
    QCOMPARE(path,
             QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation))
                 .filePath(QStringLiteral("profiles.json")));
    QVERIFY(path.endsWith(QStringLiteral("/kscaling/profiles.json")));
}

void SettingsTest::load_missingFile_isEmpty()
{
    QVERIFY(!QFile::exists(Settings::filePath()));
    const Settings::LoadResult result = Settings::load();
    QVERIFY(result.ok);
    QVERIFY(result.error.isEmpty());
    QVERIFY(result.outputs.isEmpty());
}

void SettingsTest::load_missingOutputs_isEmpty()
{
    const QByteArray json = R"({"version":1})";
    QVERIFY(QDir().mkpath(QFileInfo(Settings::filePath()).absolutePath()));
    QFile file(Settings::filePath());
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QCOMPARE(file.write(json), qsizetype(json.size()));
    file.close();

    const Settings::LoadResult result = Settings::load();
    QVERIFY(result.ok);
    QVERIFY(result.outputs.isEmpty());
}

void SettingsTest::load_unknownVersion_isError()
{
    const QByteArray json = R"({"version":2,"outputs":{}})";
    QVERIFY(QDir().mkpath(QFileInfo(Settings::filePath()).absolutePath()));
    QFile file(Settings::filePath());
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QCOMPARE(file.write(json), qsizetype(json.size()));
    file.close();

    const Settings::LoadResult result = Settings::load();
    QVERIFY(!result.ok);
    QVERIFY(!result.error.isEmpty());
    QVERIFY(result.outputs.isEmpty());
}

void SettingsTest::load_integerConnectorKey_isError()
{
    const QByteArray json = R"({"version":1,"outputs":{"67":{"preset":"perfect","mode":[4096,2304],"hz":60,"originalScale":1.0}}})";
    QVERIFY(QDir().mkpath(QFileInfo(Settings::filePath()).absolutePath()));
    QFile file(Settings::filePath());
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QCOMPARE(file.write(json), qsizetype(json.size()));
    file.close();

    const Settings::LoadResult result = Settings::load();
    QVERIFY(!result.ok);
    QVERIFY(!result.error.isEmpty());
}

void SettingsTest::save_roundtrip_dp3Profile()
{
    SavedProfile profile;
    profile.preset = QStringLiteral("perfect");
    profile.mode = QSize(4096, 2304);
    profile.hz = 60;
    profile.originalScale = 1.0;

    Settings::ProfileMap outputs;
    outputs.insert(QStringLiteral("DP-3"), profile);

    const Settings::SaveResult saved = Settings::save(outputs);
    QVERIFY(saved.ok);
    QVERIFY(saved.error.isEmpty());

    const Settings::LoadResult loaded = Settings::load();
    QVERIFY(loaded.ok);
    QCOMPARE(loaded.outputs.size(), 1);
    QVERIFY(loaded.outputs.contains(QStringLiteral("DP-3")));
    const SavedProfile& got = loaded.outputs.value(QStringLiteral("DP-3"));
    QCOMPARE(got.preset, QStringLiteral("perfect"));
    QCOMPARE(got.mode, QSize(4096, 2304));
    QCOMPARE(got.hz, 60.0);
    QCOMPARE(got.originalScale, 1.0);
}

void SettingsTest::save_integerConnectorKey_isError()
{
    SavedProfile profile;
    profile.preset = QStringLiteral("perfect");
    profile.mode = QSize(4096, 2304);
    profile.hz = 60;
    profile.originalScale = 1.0;

    Settings::ProfileMap outputs;
    outputs.insert(QStringLiteral("67"), profile);

    const Settings::SaveResult saved = Settings::save(outputs);
    QVERIFY(!saved.ok);
    QVERIFY(!saved.error.isEmpty());
    QVERIFY(!QFile::exists(Settings::filePath()));
}

void SettingsTest::save_doesNotWriteR3Keys()
{
    SavedProfile profile;
    profile.preset = QStringLiteral("perfect");
    profile.mode = QSize(4096, 2304);
    profile.hz = 60;
    profile.originalScale = 1.0;

    Settings::ProfileMap outputs;
    outputs.insert(QStringLiteral("DP-3"), profile);
    QVERIFY(Settings::save(outputs).ok);

    QFile file(Settings::filePath());
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    QVERIFY(!root.contains(QStringLiteral("applyOnLogin")));
    QVERIFY(!root.contains(QStringLiteral("applyDelayMs")));
    QCOMPARE(root.value(QStringLiteral("version")).toInt(), 1);
    QVERIFY(root.contains(QStringLiteral("outputs")));
}

QTEST_GUILESS_MAIN(SettingsTest)
#include "SettingsTest.moc"
