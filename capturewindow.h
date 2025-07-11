#ifndef CAPTUREWINDOW_H
#define CAPTUREWINDOW_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QKeyEvent>
#include <QDir>

class CaptureWindow : public QDialog
{
    Q_OBJECT

public:
    explicit CaptureWindow(QWidget *parent = nullptr,
                          const QString &surfacePath = QString(),
                          int imagesInX = 3,
                          int imagesInY = 3,
                          const QVector<int> &sequence = QVector<int>());
    ~CaptureWindow();

    QStringList getCapturedImages() const { return capturedImages; }

signals:
    void imageCaptured(const QString &imagePath);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateCameraFeed();
    void handleNetworkReply(QNetworkReply *reply);
    void captureImage();
    void finishCapturing();

private:
    void setupUI();
    void connectToCamera();
    void updateStatusLabel();
    QString getCurrentCoordinates() const;
    void saveImage(const QImage &image);
    void saveSettings();
    void drawReferenceBox(QPainter &painter);

    QLabel *imageLabel;
    QLabel *statusLabel;
    QPushButton *captureButton;
    QPushButton *finishButton;
    
    QNetworkAccessManager *networkManager;
    QTimer *updateTimer;
    
    QString surfacePath;
    int imagesInX;
    int imagesInY;
    QVector<int> sequence;
    int currentCaptureIndex;
    QImage lastFrame;
    
    const QString cameraUrl = "http://192.168.43.1:8080/shot.jpg";
    const int referenceBoxWidth = 970;
    const int referenceBoxHeight = 686;
    bool cameraConnected;
    
    QStringList capturedImages;
    QString lastSavedImagePath;
};

#endif // CAPTUREWINDOW_H 