#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QXmlStreamReader>

namespace {
const QString remoteDirectory = QStringLiteral("root@192.168.1.1:/root/");
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , consoleBoard(new QPlainTextEdit(this))
    , consoleSppz(new QPlainTextEdit(this))
    , tempFolder(QDir::homePath() + "/ssh-xml-manager/downloads")
    , SppzFolder(QDir::homePath() + "/ssh-xml-manager/files")
{
    ui->setupUi(this);
    ui->scrollArea_Board->setWidget(consoleBoard);
    ui->scrollArea_Sppz->setWidget(consoleSppz);
    consoleBoard->setReadOnly(true);
    consoleSppz->setReadOnly(true);
    ui->boardPath->setPlainText(tempFolder);
    ui->sppzPath->setPlainText(SppzFolder);
    ui->fileName->setPlainText("pi.xml");
}

MainWindow::~MainWindow()
{
    delete ui;
}

QString MainWindow::selectedFileName()
{
    const QString name = ui->fileName->toPlainText().trimmed();
    // A single remote file name, without shell metacharacters or path traversal.
    static const QRegularExpression validName("^[A-Za-z0-9_][A-Za-z0-9_.-]*$");
    if (!validName.match(name).hasMatch()) {
        consoleBoard->appendPlainText("Enter a file name using letters, digits, _, - or . (no path).");
        showResult("Invalid file name", false);
        return QString();
    }
    return name;
}

bool MainWindow::runScp(const QStringList &arguments)
{
    QProcess process;
    // Keep arguments separate so local paths containing spaces stay intact.
    process.start("scp", QStringList{"-B", "-o", "ConnectTimeout=10", "--"} + arguments);
    if (!process.waitForStarted()) {
        consoleBoard->appendPlainText("Could not start scp: " + process.errorString());
        return false;
    }
    if (!process.waitForFinished(30000)) {
        process.kill();
        process.waitForFinished();
        consoleBoard->appendPlainText("SCP timed out or failed to finish.");
        consoleBoard->appendPlainText(QString::fromLocal8Bit(process.readAllStandardError()));
        return false;
    }
    consoleBoard->appendPlainText(QString::fromLocal8Bit(process.readAllStandardOutput()));
    consoleBoard->appendPlainText(QString::fromLocal8Bit(process.readAllStandardError()));
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        consoleBoard->appendPlainText("SCP failed (exit code " + QString::number(process.exitCode()) + ").");
        return false;
    }
    return true;
}

bool MainWindow::getFile(const QString &fileName)
{
    if (fileName.isEmpty())
        return false;
    if (tempFolder.isEmpty() || !QDir().mkpath(tempFolder)) {
        consoleBoard->appendPlainText("Cannot create the download folder.");
        return false;
    }
    return runScp({remoteDirectory + fileName, QDir(tempFolder).absolutePath() + "/"});
}

bool MainWindow::changeFile(const QString &fileName)
{
    if (fileName.isEmpty())
        return false;
    const QString source = QDir(SppzFolder).absoluteFilePath(fileName);
    QByteArray data;
    if (!readFile(source, data, consoleSppz))
        return false;
    return runScp({source, remoteDirectory + fileName});
}

void MainWindow::showResult(const QString &message, bool success)
{
    ui->compareLabel->setText(message);
    ui->compareLabel->setStyleSheet(success
        ? "background-color:green; font-size:14px; font:bold; color:black"
        : "background-color:red; font-size:14px; font:bold; color:white");
}

void MainWindow::on_giveFiles_clicked()
{
    const bool success = changeFile(selectedFileName());
    showResult(success ? "File updated" : "Update failed", success);
}

bool MainWindow::readFile(const QString &path, QByteArray &data, QPlainTextEdit *console)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        console->appendPlainText("Cannot read " + path + ": " + file.errorString());
        return false;
    }
    data = file.readAll();
    if (file.error() != QFile::NoError) {
        console->appendPlainText("Cannot read " + path + ": " + file.errorString());
        return false;
    }
    return true;
}

void MainWindow::readPi(const QByteArray &fileData, QPlainTextEdit *console)
{
    console->clear();
    QXmlStreamReader xml(fileData);
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement())
            continue;
        QString attributes;
        const auto values = xml.attributes();
        for (const QString &name : QStringList{"ClassTag", "id", "filename", "RUS", "CharGroupId", "Value"}) {
            if (values.hasAttribute(name))
                attributes += name + " = " + values.value(name).toString() + "; ";
        }
        if (!attributes.isEmpty())
            console->appendPlainText(attributes);
    }
    if (xml.hasError())
        console->appendPlainText("Invalid XML at line " + QString::number(xml.lineNumber()) + ": " + xml.errorString());
}

void MainWindow::readTempPi()
{
    const QString name = selectedFileName();
    if (!getFile(name)) {
        showResult("Download failed", false);
        return;
    }
    QByteArray data;
    if (readFile(QDir(tempFolder).filePath(name), data, consoleBoard))
        readPi(data, consoleBoard);
    else
        showResult("Cannot read download", false);
}

void MainWindow::readSppzPi()
{
    const QString name = selectedFileName();
    if (name.isEmpty())
        return;
    QByteArray data;
    if (readFile(QDir(SppzFolder).filePath(name), data, consoleSppz))
        readPi(data, consoleSppz);
    else
        showResult("Cannot read local file", false);
}

void MainWindow::on_readTempPi_clicked()
{
    readTempPi();
}

void MainWindow::on_readSppzPi_clicked()
{
    readSppzPi();
}

void MainWindow::on_compareButton_clicked()
{
    const QString name = selectedFileName();
    if (name.isEmpty())
        return;
    QByteArray downloaded, local;
    if (!readFile(QDir(tempFolder).filePath(name), downloaded, consoleBoard)
        || !readFile(QDir(SppzFolder).filePath(name), local, consoleSppz)) {
        showResult("Cannot compare files", false);
        return;
    }
    const bool equal = downloaded == local;
    showResult(equal ? "Files are equal" : "Files differ", equal);
}

void MainWindow::on_changeBoardPathButton_clicked()
{
    const QString path = ui->boardPath->toPlainText().trimmed();
    if (!path.isEmpty())
        tempFolder = QDir(path).absolutePath();
    else
        showResult("Enter a download folder", false);
}

void MainWindow::on_changeSPPZPathButton_clicked()
{
    const QString path = ui->sppzPath->toPlainText().trimmed();
    if (!path.isEmpty())
        SppzFolder = QDir(path).absolutePath();
    else
        showResult("Enter a local folder", false);
}

void MainWindow::on_getFileButton_clicked()
{
    const bool success = getFile(selectedFileName());
    showResult(success ? "File downloaded" : "Download failed", success);
}
