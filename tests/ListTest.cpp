// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#include "cli/Cli.h"
#include "MockDisplayBackend.h"

#include <QTest>

class ListTest : public QObject
{
    Q_OBJECT

private slots:
    void fixtureA_printsColumns();
    void fixtureA_printsGoldenPresetRows();
    void incapable_printsIncapableAndConnectorName_notNumericId();

private:
    static OutputSnapshot fixtureADp3();
    static OutputSnapshot incapableHdmi();
};

OutputSnapshot ListTest::fixtureADp3()
{
    OutputSnapshot snapshot;
    snapshot.name = QStringLiteral("DP-3");
    snapshot.connected = true;
    snapshot.customModesCapable = true;
    snapshot.scale = 2.00;
    snapshot.native = Mode{{2560, 1440}, 164.96, {}};
    snapshot.current = Mode{{4096, 2304}, 59.93, {}};
    return snapshot;
}

OutputSnapshot ListTest::incapableHdmi()
{
    OutputSnapshot snapshot;
    snapshot.name = QStringLiteral("HDMI-A-1");
    snapshot.connected = true;
    snapshot.customModesCapable = false;
    snapshot.scale = 1.00;
    snapshot.native = Mode{{1920, 1080}, 60.00, {}};
    snapshot.current = snapshot.native;
    return snapshot;
}

void ListTest::fixtureA_printsColumns()
{
    MockDisplayBackend mock;
    mock.injectOutput(fixtureADp3());

    const QString text = Cli::formatList(mock.list());
    const QString header = text.split(QLatin1Char('\n')).constFirst();
    QCOMPARE(header, QStringLiteral("DP-3  capable  native 2560x1440  current 4096x2304 @ 59.93  scale 2.00"));
}

void ListTest::fixtureA_printsGoldenPresetRows()
{
    MockDisplayBackend mock;
    mock.injectOutput(fixtureADp3());

    const QString text = Cli::formatList(mock.list());
    const QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    QCOMPARE(lines.value(1), QStringLiteral("  perfect    looks 2048x1152  mode 4096x2304 @ 120"));
    QCOMPARE(lines.value(2), QStringLiteral("  max-space  looks 2560x1440  mode 5120x2880 @ 60"));
    QCOMPARE(lines.value(3), QStringLiteral("  comfort    looks 1600x900   mode 3200x1800 @ 120"));
    QCOMPARE(lines.value(4), QStringLiteral("  large      looks 1440x810   mode 2880x1620 @ 120"));
}

void ListTest::incapable_printsIncapableAndConnectorName_notNumericId()
{
    MockDisplayBackend mock;
    mock.injectOutput(incapableHdmi());

    const QString nativeId = mock.list().snapshots.at(0).native.id;
    QVERIFY(!nativeId.isEmpty());

    const QString text = Cli::formatList(mock.list());
    const QString header = text.split(QLatin1Char('\n')).constFirst();
    QVERIFY(header.startsWith(QStringLiteral("HDMI-A-1  incapable")));
    QVERIFY(!header.startsWith(nativeId));
    QVERIFY(!header.at(0).isDigit());
}

QTEST_APPLESS_MAIN(ListTest)
#include "ListTest.moc"
