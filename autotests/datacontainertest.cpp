/*
    SPDX-FileCopyrightText: 2026 Nekto Oleg <27015@riseup.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QTest>

#include <Plasma5Support/DataContainer>
#include <Plasma5Support/DataEngine>

#include <functional>
#include <utility>

static const auto s_source = QStringLiteral("source");
static const auto s_key = QStringLiteral("key");

class TestContainer : public Plasma5Support::DataContainer
{
public:
    using DataContainer::checkForUpdate;
    using DataContainer::DataContainer;
};

class TestEngine : public Plasma5Support::DataEngine
{
public:
    enum class Poll {
        Ignore,
        SetData,
        // Mirrors TimeEngine::updateSourceEvent()
        SetDataAndForceUpdate,
    };

    TestEngine()
        : m_container(new TestContainer(this))
    {
        m_container->setObjectName(s_source);
        m_container->setData(s_key, 0);
        addSource(m_container);
    }

    TestContainer *container() const
    {
        return m_container;
    }

    Poll poll = Poll::Ignore;

protected:
    bool updateSourceEvent(const QString &source) override
    {
        if (poll == Poll::Ignore) {
            return false;
        }

        setData(source, s_key, ++m_value);
        if (poll == Poll::SetDataAndForceUpdate) {
            m_container->forceImmediateUpdate();
        }
        return true;
    }

private:
    TestContainer *const m_container;
    int m_value = 0;
};

class Receiver : public QObject
{
    Q_OBJECT

public:
    int updates = 0;
    std::function<void()> onNextUpdate;

public Q_SLOTS:
    void dataUpdated(const QString &, const Plasma5Support::DataEngine::Data &)
    {
        ++updates;
        if (onNextUpdate) {
            std::exchange(onNextUpdate, nullptr)();
        }
    }
};

class DataContainerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void relayDeletedInCheckQueueing_data();
    void relayDeletedInCheckQueueing();
    void relayDeletedInTimerEvent();
    void relayDeletedInUpdateRequest();
    void relayDeletedInForceImmediateUpdate();
    void otherRelayDeletedInCheckForUpdate();
};

void DataContainerTest::relayDeletedInCheckQueueing_data()
{
    QTest::addColumn<bool>("reconnect");

    QTest::newRow("disconnect") << false;
    QTest::newRow("reconnect") << true;
}

void DataContainerTest::relayDeletedInCheckQueueing()
{
    QFETCH(bool, reconnect);

    TestEngine engine;
    Receiver receiver;
    engine.connectSource(s_source, &receiver, 50);
    QCOMPARE(receiver.updates, 1);

    receiver.onNextUpdate = [&] {
        if (reconnect) {
            engine.connectSource(s_source, &receiver, 100);
        } else {
            engine.disconnectSource(s_source, &receiver);
        }
    };
    engine.container()->setData(s_key, 1);
    engine.container()->checkForUpdate();

    QCOMPARE(receiver.updates, 2);
    QCOMPARE(engine.container()->visualizationIsConnected(&receiver), reconnect);
}

void DataContainerTest::relayDeletedInTimerEvent()
{
    TestEngine engine;
    engine.poll = TestEngine::Poll::SetData;
    Receiver receiver;
    engine.connectSource(s_source, &receiver, 50);
    QCOMPARE(receiver.updates, 1);

    receiver.onNextUpdate = [&] {
        engine.disconnectSource(s_source, &receiver);
    };

    QTRY_COMPARE(receiver.updates, 2);
    QVERIFY(!engine.container()->visualizationIsConnected(&receiver));
}

void DataContainerTest::relayDeletedInUpdateRequest()
{
    TestEngine engine;
    engine.poll = TestEngine::Poll::SetDataAndForceUpdate;
    Receiver receiver;
    engine.connectSource(s_source, &receiver, 50);
    QCOMPARE(receiver.updates, 1);

    receiver.onNextUpdate = [&] {
        engine.disconnectSource(s_source, &receiver);
    };

    QTRY_COMPARE(receiver.updates, 2);
    QVERIFY(!engine.container()->visualizationIsConnected(&receiver));
}

void DataContainerTest::relayDeletedInForceImmediateUpdate()
{
    TestEngine engine;
    Receiver first;
    Receiver second;
    engine.connectSource(s_source, &first, 50);
    engine.connectSource(s_source, &second, 100);
    QCOMPARE(first.updates, 1);
    QCOMPARE(second.updates, 1);

    second.onNextUpdate = [&] {
        engine.disconnectSource(s_source, &second);
    };
    engine.container()->forceImmediateUpdate();

    QCOMPARE(first.updates, 2);
    QCOMPARE(second.updates, 2);
    QVERIFY(!engine.container()->visualizationIsConnected(&second));
}

void DataContainerTest::otherRelayDeletedInCheckForUpdate()
{
    TestEngine engine;
    Receiver first;
    Receiver second;
    engine.connectSource(s_source, &first, 50);
    engine.connectSource(s_source, &second, 100);
    QCOMPARE(first.updates, 1);
    QCOMPARE(second.updates, 1);

    // Relays are ordered by interval, so the relay of the first receiver runs first
    first.onNextUpdate = [&] {
        engine.disconnectSource(s_source, &second);
    };
    engine.container()->setData(s_key, 1);
    engine.container()->checkForUpdate();

    QCOMPARE(first.updates, 2);
    QCOMPARE(second.updates, 1);
    QVERIFY(!engine.container()->visualizationIsConnected(&second));
}

QTEST_GUILESS_MAIN(DataContainerTest)

#include "datacontainertest.moc"
