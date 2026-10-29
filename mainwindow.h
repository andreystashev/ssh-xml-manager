#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QStringList>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_readTempPi_clicked();
    void on_readSppzPi_clicked();
    void on_compareButton_clicked();
    void on_giveFiles_clicked();
    void on_changeBoardPathButton_clicked();
    void on_changeSPPZPathButton_clicked();
    void on_getFileButton_clicked();

private:
    QString selectedFileName();
    bool readFile(const QString &path, QByteArray &data, QPlainTextEdit *console);
    void readPi(const QByteArray &fileData, QPlainTextEdit *console);
    void readTempPi();
    void readSppzPi();
    bool runScp(const QStringList &arguments);
    bool getFile(const QString &fileName);
    bool changeFile(const QString &fileName);
    void showResult(const QString &message, bool success);

    Ui::MainWindow *ui;
    QPlainTextEdit *consoleBoard;
    QPlainTextEdit *consoleSppz;
    QString tempFolder;
    QString SppzFolder;
};
#endif // MAINWINDOW_H
