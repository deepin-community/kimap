/*
   SPDX-FileCopyrightText: 2011 Andras Mantia <amantia@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <QTest>

#include "kimap/getquotarootjob.h"
#include "kimap/session.h"
#include "kimaptest/fakeserver.h"

#include <QTest>

Q_DECLARE_METATYPE(QList<qint64>)

class QuotaRootJobTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void testGetQuotaRoot_data()
    {
        QTest::addColumn<QString>("mailbox");
        QTest::addColumn<QList<QByteArray>>("roots");
        QTest::addColumn<QList<QByteArray>>("resources");
        QTest::addColumn<QList<qint64>>("usages");
        QTest::addColumn<QList<qint64>>("limits");
        QTest::addColumn<QList<QByteArray>>("scenario");

        QList<QByteArray> roots;
        QList<QByteArray> resources;
        QList<qint64> usages;
        QList<qint64> limits;
        QList<QByteArray> scenario;
        roots << "";
        resources << "STORAGE";
        limits << 512;
        usages << 10;
        scenario << FakeServer::preauth() << "C: A000001 GETQUOTAROOT \"INBOX\""
                 << "S: * QUOTAROOT INBOX \"\" "
                 << "S: * QUOTA \"\" (STORAGE 10 512)"
                 << "S: A000001 OK GETQUOTA completed";
        QTest::newRow("one root, one resource") << "INBOX" << roots << resources << usages << limits << scenario;

        roots.clear();
        resources.clear();
        usages.clear();
        limits.clear();
        scenario.clear();
        roots << "";
        resources << "STORAGE"
                  << "MESSAGE";
        usages << 10 << 8221;
        limits << 512 << 20000;
        scenario << FakeServer::preauth() << "C: A000001 GETQUOTAROOT \"INBOX\""
                 << "S: * QUOTAROOT INBOX \"\" "
                 << "S: * QUOTA \"\" (STORAGE 10 512)"
                 << "S: * QUOTA \"\" ( MESSAGE 8221 20000 ) "
                 << "S: A000001 OK GETQUOTA completed";
        QTest::newRow("one root, multiple resource") << "INBOX" << roots << resources << usages << limits << scenario;

        roots.clear();
        resources.clear();
        usages.clear();
        limits.clear();
        scenario.clear();
        roots << "root1"
              << "root2";
        resources << "STORAGE"
                  << "MESSAGE";
        usages << 10 << 8221 << 30 << 100;
        limits << 512 << 20000 << 5124 << 120000;
        scenario << FakeServer::preauth() << "C: A000001 GETQUOTAROOT \"INBOX\""
                 << "S: * QUOTAROOT INBOX \"root1\" root2 "
                 << "S: * QUOTA \"root1\" (STORAGE 10 512)"
                 << "S: * QUOTA \"root1\" ( MESSAGE 8221 20000 ) "
                 << "S: * QUOTA \"root2\" ( MESSAGE 100 120000 ) "
                 << "S: * QUOTA \"root2\" (STORAGE 30 5124)"
                 << "S: A000001 OK GETQUOTA completed";
        QTest::newRow("multiple roots, multiple resource") << "INBOX" << roots << resources << usages << limits << scenario;

        roots.clear();
        resources.clear();
        usages.clear();
        limits.clear();
        scenario.clear();
        roots << "";
        resources << "STORAGE"
                  << "MESSAGE";
        usages << 10 << 8221;
        limits << 512 << 20000;
        scenario << FakeServer::preauth() << "C: A000001 GETQUOTAROOT \"INBOX\""
                 << "S: * QUOTAROOT INBOX"
                 << "S: * QUOTA (STORAGE 10 512)"
                 << "S: * QUOTA ( MESSAGE 8221 20000 ) "
                 << "S: A000001 OK GETQUOTA completed";
        QTest::newRow("no rootname, multiple resource") << "INBOX" << roots << resources << usages << limits << scenario;

        roots.clear();
        resources.clear();
        usages.clear();
        limits.clear();
        scenario.clear();
        roots << "";
        resources << "STORAGE";
        limits << 512;
        usages << 10;
        scenario << FakeServer::preauth() << "C: A000001 GETQUOTAROOT \"INBOX\""
                 << "S: * QUOTAROOT INBOX \"\" "
                 << "S: * QUOTA (STORAGE 10 512)"
                 << "S: A000001 OK GETQUOTA completed";
        QTest::newRow("no rootname in QUOTA, one resource") << "INBOX" << roots << resources << usages << limits << scenario;

        roots.clear();
        resources.clear();
        usages.clear();
        limits.clear();
        scenario.clear();
        roots << "";
        resources << "STORAGE";
        limits << 512;
        usages << 10;
        scenario << FakeServer::preauth() << "C: A000001 GETQUOTAROOT \"INBOX\""
                 << "S: * QUOTAROOT INBOX \"root1\" "
                 << "S: * QUOTA \"root2\" (STORAGE 10 512)"
                 << "S: A000001 OK GETQUOTA completed";
        QTest::newRow("rootname missmatch") << "INBOX" << roots << resources << usages << limits << scenario;
    }

    void testGetQuotaRoot()
    {
        QFETCH(QString, mailbox);
        QFETCH(QList<QByteArray>, roots);
        QFETCH(QList<QByteArray>, resources);
        QFETCH(QList<qint64>, usages);
        QFETCH(QList<qint64>, limits);
        QFETCH(QList<QByteArray>, scenario);

        FakeServer fakeServer;
        fakeServer.setScenario(scenario);
        fakeServer.startAndWait();

        KIMAP::Session session(QStringLiteral("127.0.0.1"), 5989);

        auto job = new KIMAP::GetQuotaRootJob(&session);
        job->setMailBox(mailbox);
        bool result = job->exec();
        QEXPECT_FAIL("bad", "Expected failure on BAD response", Continue);
        QEXPECT_FAIL("no", "Expected failure on NO response", Continue);
        QVERIFY(result);
        QEXPECT_FAIL("rootname missmatch", "Expected failure on rootname missmatch in QUOTAROOT and QUOTA response", Abort);
        QCOMPARE(job->roots(), roots);
        for (int rootIdx = 0; rootIdx < roots.size(); rootIdx++) {
            const QByteArray &root = roots[rootIdx];
            for (int i = 0; i < resources.size(); i++) {
                int idx = i + rootIdx * roots.size();
                QByteArray resource = resources[i];
                QCOMPARE(job->limit(root, resource), limits[idx]);
                QCOMPARE(job->usage(root, resource), usages[idx]);
            }
        }

        fakeServer.quit();
    }
};

QTEST_GUILESS_MAIN(QuotaRootJobTest)

#include "quotarootjobtest.moc"
