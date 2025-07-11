#include "defectdetector.h"
#include <QtCore/QDir>
#include <QtCore/QFile>

DefectDetector::DefectDetector(QObject *parent)
    : QObject(parent)
    , detectionProcess(nullptr)
    , modelInitialized(false)
{
    pythonScriptPath = QDir::currentPath() + "/scripts/defect_detector.py";
}

DefectDetector::~DefectDetector()
{
    if (detectionProcess) {
        detectionProcess->close();
        delete detectionProcess;
    }
}

void DefectDetector::initializeDetectionProcess()
{
    if (detectionProcess) {
        detectionProcess->close();
        delete detectionProcess;
    }

    detectionProcess = new QProcess(this);
    detectionProcess->setProgram("/usr/local/bin/python3");
    detectionProcess->setProcessChannelMode(QProcess::MergedChannels);

    // Check if the Python script exists
    if (!QFile::exists(pythonScriptPath)) {
        QString error = "Python script not found at: " + pythonScriptPath;
        emit statusMessage(error);
        emit modelInitializationFailed(error);
        return;
    }

    emit statusMessage("Starting Python process with script: " + pythonScriptPath);

    // Set up process arguments
    QStringList arguments;
    arguments << pythonScriptPath;
    detectionProcess->setArguments(arguments);

    // Connect signals
    connect(detectionProcess, &QProcess::readyRead, this, &DefectDetector::handleProcessOutput);
    connect(detectionProcess, &QProcess::errorOccurred, this, &DefectDetector::handleProcessError);
    connect(detectionProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &DefectDetector::handleProcessFinished);

    // Start the process
    detectionProcess->start();
    
    if (!detectionProcess->waitForStarted()) {
        QString error = "Failed to start detection process";
        emit statusMessage(error);
        emit modelInitializationFailed(error);
        return;
    }
}

void DefectDetector::handleProcessOutput()
{
    if (!detectionProcess) return;

    QString output = QString::fromUtf8(detectionProcess->readAll()).trimmed();
    emit statusMessage(output);  // Emit all output as status messages
    qDebug() << "Python output:" << output;

    if (output.contains("[SUCCESS] Model loaded successfully")) {
        modelInitialized = true;
        emit modelInitializationComplete();
    }
    else if (output.contains("[ERROR]")) {
        emit modelInitializationFailed(output);
    }
    else if (output.contains("[SUCCESS] Detection results saved")) {
        // Parse and emit detection results
        QStringList results;
        for (const QString &line : output.split('\n')) {
            if (line.contains("[INFO]")) {
                results.append(line.mid(line.indexOf("[INFO]") + 7));
            }
        }
        emit detectionComplete(results);
    }
}

void DefectDetector::handleProcessError(QProcess::ProcessError error)
{
    QString errorMessage;
    switch (error) {
        case QProcess::FailedToStart:
            errorMessage = "Failed to start Python process";
            break;
        case QProcess::Crashed:
            errorMessage = "Python process crashed";
            break;
        case QProcess::Timedout:
            errorMessage = "Process timed out";
            break;
        case QProcess::WriteError:
            errorMessage = "Write error occurred";
            break;
        case QProcess::ReadError:
            errorMessage = "Read error occurred";
            break;
        default:
            errorMessage = "Unknown error occurred";
    }
    emit statusMessage(errorMessage);
    emit modelInitializationFailed(errorMessage);
}

void DefectDetector::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::CrashExit || exitCode != 0) {
        modelInitialized = false;
        QString error = "Process terminated unexpectedly";
        emit statusMessage(error);
        emit modelInitializationFailed(error);
    }
}

void DefectDetector::writeToProcess(const QString &command)
{
    if (!detectionProcess || !modelInitialized) {
        emit statusMessage("Cannot send command - process not ready");
        return;
    }

    detectionProcess->write((command + "\n").toUtf8());
    detectionProcess->waitForBytesWritten();
}

void DefectDetector::detectImage(const QString &imagePath)
{
    if (!modelInitialized) {
        emit statusMessage("Model not initialized - cannot detect defects");
        return;
    }

    QString command = QString("detect %1").arg(imagePath);
    writeToProcess(command);
} 