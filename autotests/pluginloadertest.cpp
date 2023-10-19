/*
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "pluginloadertest.h"

#include <QDebug>
#include <QSignalSpy>
#include <qtest.h>

#include <KPluginMetaData>

#include <Plasma5Support/DataEngineConsumer>
#include <Plasma5Support/PluginLoader>

QTEST_MAIN(PluginTest)

PluginTest::PluginTest()
    : m_buildonly(false)
{
}

void PluginTest::listEngines()
{
    QList<KPluginMetaData> plugins = Plasma5Support::PluginLoader::listDataEngineMetaData();
    qDebug() << " Found " << plugins.count() << " DataEngines";
    // Switch to true in order to let tests pass, this test usually will only
    // work with plugins installed, but there aren't any in plasma-framework
    m_buildonly = plugins.count() == 0;
    QVERIFY(plugins.count() > 0 || m_buildonly);
}

static const auto source = QStringLiteral("Europe/Sofia");

void EngineTest::dataUpdated(const QString &s, const Plasma5Support::DataEngine::Data &data)
{
    QVERIFY(source == s);
    QVERIFY(data[QStringLiteral("Timezone")] == source);
}

void PluginTest::loadDataEngine()
{
    if (m_buildonly) {
        return;
    }
    QPointer<Plasma5Support::DataEngine> engine, nullEngine;
    {
        Plasma5Support::DataEngineConsumer consumer;
        engine = consumer.dataEngine(QStringLiteral("time"));
        nullEngine = consumer.dataEngine(QStringLiteral("noop"));
        QVERIFY(nullEngine && engine);
        QVERIFY(!nullEngine->isValid());
        QVERIFY(engine->isValid());
        {
            EngineTest test;
            QSignalSpy spy(engine, SIGNAL(sourceAdded(QString)));
            engine->connectSource(source, &test);
            QCOMPARE_GT(0, spy.count()); // Should be emitted immediately
            QVERIFY(!engine->isEmpty());
        }
        QSignalSpy spy(engine, SIGNAL(sourceRemoved(QString)));
        QVERIFY(spy.wait());
        QVERIFY(engine->isEmpty());
    }
    QVERIFY(!nullEngine.isNull() && engine.isNull());
}

#include "moc_pluginloadertest.cpp"
