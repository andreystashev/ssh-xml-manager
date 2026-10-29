#include "mainwindow.h"

#include <QDir>
#include <QFile>
#include <QLabel>
#include <QScrollArea>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QtTest>

class MainWindowTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir directory;
    MainWindow *window = nullptr;
    QByteArray originalPath = qgetenv("PATH");
    QString downloads;
    QString replacements;
    QString log;

    void write(const QString &path, const QByteArray &contents)
    {
        QFile file(path);
        QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(file.errorString()));
        QCOMPARE(file.write(contents), qint64(contents.size()));
    }

    void invoke(const char *slot)
    {
        QVERIFY(QMetaObject::invokeMethod(window, slot));
    }

    QString result() const
    {
        return window->findChild<QLabel *>("compareLabel")->text();
    }

    QStringList scpArguments()
    {
        QFile file(log);
        if (!file.open(QIODevice::ReadOnly))
            return {};
        return QString::fromUtf8(file.readAll()).trimmed().split('\n');
    }

private slots:
    void initTestCase()
    {
        QVERIFY(directory.isValid());
        const QString bin = directory.filePath("bin");
        QVERIFY(QDir().mkpath(bin));
        const QString scp = bin + "/scp";
        write(scp, "#!/bin/sh\nprintf '%s\\n' \"$@\" > \"$SCP_TEST_LOG\"\n"
                   "if [ \"$SCP_TEST_EXIT\" != 0 ]; then exit \"$SCP_TEST_EXIT\"; fi\n"
                   "if [ -n \"$SCP_TEST_SOURCE\" ]; then cp \"$SCP_TEST_SOURCE\" \"$6\"; fi\n");
        QVERIFY(QFile::setPermissions(scp, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        qputenv("PATH", bin.toLocal8Bit() + ":" + originalPath);
        log = directory.filePath("scp.log");
        qputenv("SCP_TEST_LOG", log.toLocal8Bit());
    }

    void init()
    {
        downloads = directory.filePath("download folder");
        replacements = directory.filePath("replacement folder");
        QVERIFY(QDir().mkpath(downloads));
        QVERIFY(QDir().mkpath(replacements));
        QFile::remove(log);
        QFile::remove(downloads + "/settings.xml");
        QFile::remove(replacements + "/settings.xml");
        qputenv("SCP_TEST_EXIT", "0");
        qunsetenv("SCP_TEST_SOURCE");
        window = new MainWindow;
        window->findChild<QTextEdit *>("boardPath")->setPlainText(downloads);
        window->findChild<QTextEdit *>("sppzPath")->setPlainText(replacements);
        window->findChild<QTextEdit *>("fileName")->setPlainText("settings.xml");
        invoke("on_changeBoardPathButton_clicked");
        invoke("on_changeSPPZPathButton_clicked");
    }

    void cleanup()
    {
        delete window;
        window = nullptr;
    }

    void cleanupTestCase()
    {
        qputenv("PATH", originalPath);
        qunsetenv("SCP_TEST_LOG");
        qunsetenv("SCP_TEST_EXIT");
        qunsetenv("SCP_TEST_SOURCE");
    }

    void missingFilesAreNotEqual()
    {
        invoke("on_compareButton_clicked");
        QCOMPARE(result(), QString("Cannot compare files"));
    }

    void comparesSelectedFile()
    {
        write(downloads + "/settings.xml", "<root/>\n");
        write(replacements + "/settings.xml", "<root/>\n");
        invoke("on_compareButton_clicked");
        QCOMPARE(result(), QString("Files are equal"));
        write(replacements + "/settings.xml", "<changed/>\n");
        invoke("on_compareButton_clicked");
        QCOMPARE(result(), QString("Files differ"));
        QFile::remove(replacements + "/settings.xml");
        invoke("on_compareButton_clicked");
        QCOMPARE(result(), QString("Cannot compare files"));
    }

    void uploadUsesReplacementAndReportsExitFailure()
    {
        write(replacements + "/settings.xml", "<root/>\n");
        qputenv("SCP_TEST_EXIT", "9");
        invoke("on_giveFiles_clicked");
        QCOMPARE(result(), QString("Update failed"));
        QCOMPARE(scpArguments(), QStringList({"-B", "-o", "ConnectTimeout=10", "--",
            replacements + "/settings.xml", "root@192.168.1.1:/root/settings.xml"}));
        qputenv("SCP_TEST_EXIT", "0");
        invoke("on_giveFiles_clicked");
        QCOMPARE(result(), QString("File updated"));
    }

    void remoteReadUsesDownloadedSelectedFile()
    {
        const QString fixture = directory.filePath("settings.xml");
        write(fixture, "<root ClassTag=\"downloaded\"/>\n");
        qputenv("SCP_TEST_SOURCE", fixture.toLocal8Bit());
        invoke("on_readTempPi_clicked");
        const auto console = qobject_cast<QPlainTextEdit *>(
            window->findChild<QScrollArea *>("scrollArea_Board")->widget());
        QVERIFY(console->toPlainText().contains("ClassTag = downloaded"));
        QCOMPARE(scpArguments().last(), downloads + "/");
    }

    void failedDownloadDoesNotReadStaleCopy()
    {
        write(downloads + "/settings.xml", "<root ClassTag=\"stale\"/>\n");
        qputenv("SCP_TEST_EXIT", "1");
        invoke("on_readTempPi_clicked");
        QCOMPARE(result(), QString("Download failed"));
        const auto console = qobject_cast<QPlainTextEdit *>(
            window->findChild<QScrollArea *>("scrollArea_Board")->widget());
        QVERIFY(!console->toPlainText().contains("ClassTag = stale"));
    }

    void invalidNamesDoNotStartScp_data()
    {
        QTest::addColumn<QString>("name");
        QTest::newRow("traversal") << "../settings.xml";
        QTest::newRow("absolute") << "/etc/passwd";
        QTest::newRow("shell") << "file;whoami";
        QTest::newRow("option") << "-oProxyCommand=whoami";
        QTest::newRow("empty") << "";
    }

    void invalidNamesDoNotStartScp()
    {
        QFETCH(QString, name);
        window->findChild<QTextEdit *>("fileName")->setPlainText(name);
        invoke("on_getFileButton_clicked");
        QCOMPARE(result(), QString("Download failed"));
        QVERIFY(!QFile::exists(log));
    }
};

QTEST_MAIN(MainWindowTest)
#include "tst_mainwindow.moc"
