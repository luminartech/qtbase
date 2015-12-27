/****************************************************************************
**
** Copyright (C) 2015 Intel Corporation.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtDBus>
#include <QtTest>

#ifdef Q_OS_WIN
#  include <process.h>
#  define getpid _getpid
#else
#  include <sys/types.h>
#  include <unistd.h>
#endif

class tst_QDBusConnection_Delayed : public QObject
{
    Q_OBJECT
private slots:
    void delayedMessages();
};

class Foo : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.qtproject.tst_qdbusconnection_delayed.Foo")
public slots:
    int bar() { return 42; }
};

static bool executedOnce = false;

void tst_QDBusConnection_Delayed::delayedMessages()
{
    if (executedOnce)
        QSKIP("This test can only be executed once");
    executedOnce = true;

    int argc = 1;
    char *argv[] = { const_cast<char *>("tst_qdbusconnection_delayed"), 0 };
    QCoreApplication app(argc, argv);

    QDBusConnection session = QDBusConnection::sessionBus();
    QVERIFY(session.isConnected());
    QVERIFY(!session.baseService().isEmpty());

    QDBusConnection other = QDBusConnection::connectToBus(QDBusConnection::SessionBus, "other");
    QVERIFY(other.isConnected());
    QVERIFY(!other.baseService().isEmpty());

    QString name = "org.qtproject.tst_qdbusconnection_delayed-" +
                   QString::number(getpid());

    // acquire a name in the main session bus connection
    QVERIFY(session.registerService(name));
    QVERIFY(other.interface()->isServiceRegistered(name));

    // make an asynchronous call to a yet-unregistered object
    QDBusPendingCallWatcher pending(other.asyncCall(QDBusMessage::createMethodCall(name, "/foo", QString(), "bar")));

    // sleep the main thread without running the event loop;
    // the call must not be delivered
    QTest::qSleep(1000);
    QVERIFY(!pending.isFinished());

    // now register the object
    Foo foo;
    session.registerObject("/foo", &foo, QDBusConnection::ExportAllSlots);

    connect(&pending, &QDBusPendingCallWatcher::finished,
            &QTestEventLoop::instance(), &QTestEventLoop::exitLoop);
    QTestEventLoop::instance().enterLoop(2);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QVERIFY(pending.isFinished());
    QVERIFY2(!pending.isError(), pending.error().name().toLatin1());
    QVERIFY(!pending.reply().arguments().isEmpty());
    QCOMPARE(pending.reply().arguments().at(0), QVariant(42));
}

QTEST_APPLESS_MAIN(tst_QDBusConnection_Delayed)

#include "tst_qdbusconnection_delayed.moc"
