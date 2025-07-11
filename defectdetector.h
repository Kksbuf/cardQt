#ifndef DEFECTDETECTOR_H
#define DEFECTDETECTOR_H

#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QString>
#include <QtCore/QDebug>

class DefectDetector : public QObject
{
    Q_OBJECT

public:
    explicit DefectDetector(QObject *parent = nullptr);
    ~DefectDetector();

    void initializeDetectionProcess();
    bool isModelInitialized() const { return modelInitialized; }
    void detectImage(const QString &imagePath);

signals:
    void modelInitializationComplete();
    void modelInitializationFailed(QString error);
    void detectionComplete(const QStringList &results);
    void detectionError(QString error);
    void statusMessage(QString message);

private slots:
    void handleProcessOutput();
    void handleProcessError(QProcess::ProcessError error);
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    QProcess *detectionProcess;
    bool modelInitialized;
    QString pythonScriptPath;
    
    void writeToProcess(const QString &command);
};

#endif // DEFECTDETECTOR_H 