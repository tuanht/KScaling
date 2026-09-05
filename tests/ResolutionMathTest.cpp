// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 KScaling contributors

#include "ResolutionMath.h"

#include <QSize>
#include <QTest>

class ResolutionMathTest : public QObject
{
    Q_OBJECT

private slots:
    void fixtureA_goldens_data();
    void fixtureA_goldens();
    void fixtureB_perfect();
};

void ResolutionMathTest::fixtureA_goldens_data()
{
    QTest::addColumn<qreal>("uiScale");
    QTest::addColumn<QSize>("looksLike");
    QTest::addColumn<QSize>("canvas");
    QTest::addColumn<qreal>("hz");

    QTest::newRow("perfect") << 1.25 << QSize(2048, 1152) << QSize(4096, 2304) << 120.0;
    QTest::newRow("max-space") << 1.00 << QSize(2560, 1440) << QSize(5120, 2880) << 60.0;
    QTest::newRow("comfort") << 1.60 << QSize(1600, 900) << QSize(3200, 1800) << 120.0;
    QTest::newRow("large") << 1.778 << QSize(1440, 810) << QSize(2880, 1620) << 120.0;
}

void ResolutionMathTest::fixtureA_goldens()
{
    QFETCH(qreal, uiScale);
    QFETCH(QSize, looksLike);
    QFETCH(QSize, canvas);
    QFETCH(qreal, hz);

    const ModePlan plan = ResolutionMath::plan(QSize(2560, 1440), uiScale);
    QCOMPARE(plan.looksLike, looksLike);
    QCOMPARE(plan.canvas, canvas);
    QCOMPARE(plan.scale, 2.0);
    QCOMPARE(plan.hz, hz);
}

void ResolutionMathTest::fixtureB_perfect()
{
    const ModePlan plan = ResolutionMath::plan(QSize(1920, 1080), 1.25);
    QCOMPARE(plan.looksLike, QSize(1536, 864));
    QCOMPARE(plan.canvas, QSize(3072, 1728));
    QCOMPARE(plan.scale, 2.0);
    QCOMPARE(plan.hz, 120.0);
}

QTEST_APPLESS_MAIN(ResolutionMathTest)
#include "ResolutionMathTest.moc"
