#ifndef MOTORIZEDCAPTUREWINDOW_H
#define MOTORIZEDCAPTUREWINDOW_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QKeyEvent>
#include <QDir>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QComboBox>
#include <QHBoxLayout>

class MotorizedCaptureWindow : public QDialog
{
    Q_OBJECT

public:
    explicit MotorizedCaptureWindow(QWidget *parent = nullptr,
                                  const QString &surfacePath = QString(),
                                  int imagesInX = 3,
                                  int imagesInY = 3,
                                  const QVector<int> &sequence = QVector<int>(),
                                  bool isA4 = true);  // Add isA4 parameter
    ~MotorizedCaptureWindow();

    QStringList getCapturedImages() const { return capturedImages; }

signals:
    void imageCaptured(const QString &imagePath);

protected:
    void keyPressEvent(QKeyEvent *event) override {
        switch (event->key()) {
            case Qt::Key_Space:
                captureImage();
                break;
            case Qt::Key_Q:
                if (event->modifiers() & Qt::ControlModifier) {
                    finishCapturing();
                } else {
                    moveSteps('Z', false, stepSize); // Step -Z
                }
                break;
            case Qt::Key_Up:
                moveMotor('X', true);  // X axis up
                break;
            case Qt::Key_Down:
                moveMotor('X', false); // X axis down
                break;
            case Qt::Key_Left:
                moveMotor('Y', false); // Y axis left
                break;
            case Qt::Key_Right:
                moveMotor('Y', true);  // Y axis right
                break;
            case Qt::Key_1:
                moveMotor('Z', false); // Z axis down
                break;
            case Qt::Key_2:
                moveMotor('Z', true);  // Z axis up
                break;
            // WASD keys for step movement
            case Qt::Key_W:
                moveSteps('Y', true, stepSize);  // Step +Y
                break;
            case Qt::Key_S:
                moveSteps('Y', false, stepSize); // Step -Y
                break;
            case Qt::Key_A:
                moveSteps('X', false, stepSize); // Step -X
                break;
            case Qt::Key_D:
                moveSteps('X', true, stepSize);  // Step +X
                break;
            case Qt::Key_E:
                moveSteps('Z', true, stepSize);  // Step +Z
                break;
            default:
                QDialog::keyPressEvent(event);
        }
    }
    
    void keyReleaseEvent(QKeyEvent *event) override {
        switch (event->key()) {
            case Qt::Key_Up:
            case Qt::Key_Down:
                stopMotor('X');
                break;
            case Qt::Key_Left:
            case Qt::Key_Right:
                stopMotor('Y');
                break;
            case Qt::Key_1:
            case Qt::Key_2:
                stopMotor('Z');
                break;
            default:
                QDialog::keyReleaseEvent(event);
        }
    }
    
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateCameraFeed();
    void handleNetworkReply(QNetworkReply *reply);
    void captureImage();
    void finishCapturing();
    void handleSerialData();
    void moveMotor(char axis, bool direction);
    void moveSteps(char axis, bool direction, int steps);
    void stopMotor(char axis);
    void homeAxis(char axis);
    void refreshPortList();
    void onPortSelected(int index);
    void initializeMotors();
    void moveToNextPosition();

private:
    void setupUI();
    void connectToCamera();
    void connectToArduino();
    void updateStatusLabel();
    QString getCurrentCoordinates() const;
    void saveImage(const QImage &image);
    void saveSettings();
    void drawReferenceBox(QPainter &painter);
    void sendArduinoCommand(const QString &command);
    QHBoxLayout* setupArduinoSelection();
    QHBoxLayout* setupMotorControls();

    // UI Components
    QLabel *imageLabel;
    QLabel *statusLabel;
    QPushButton *captureButton;
    QPushButton *finishButton;
    QPushButton *homeXButton;
    QPushButton *homeYButton;
    QPushButton *homeZButton;
    QComboBox *portSelector;
    QPushButton *refreshPortButton;
    QLabel *arduinoStatusLabel;
    
    // Motor control buttons
    QPushButton *xPlusStepButton;
    QPushButton *xMinusStepButton;
    QPushButton *yPlusStepButton;
    QPushButton *yMinusStepButton;
    QPushButton *zPlusStepButton;
    QPushButton *zMinusStepButton;
    
    // Network components for camera
    QNetworkAccessManager *networkManager;
    QTimer *updateTimer;
    
    // Serial communication for Arduino
    QSerialPort *arduinoPort;
    
    // Capture settings
    QString surfacePath;
    int imagesInX;
    int imagesInY;
    QVector<int> sequence;
    int currentCaptureIndex;
    QImage lastFrame;
    
    // Camera settings
    // TODO: change camera url
    const QString cameraUrl = "http://192.168.0.7:8080/shot.jpg";
    const int referenceBoxWidth = 1100;
    const int referenceBoxHeight = 778;
    bool cameraConnected;
    
    // Motor control settings
    bool xMoving = false;
    bool yMoving = false;
    bool zMoving = false;
    const int stepSize = 5; // Number of steps to move for step movement
    
    QStringList capturedImages;
    QString lastSavedImagePath;
    
    // A4 specific settings
    bool isA4Size;
    // const int xStepSize = 265;  // Steps for X movement
    // const int yStepSize = 370;  // Steps for Y movement
    const int yHomeOffset = 30; // Y home offset
    const int zHomeOffset = -295; // Z home offset
    
    // Current position tracking
    int currentX;
    int currentY;
    
    // Helper methods
    void moveToPosition(int x, int y);
    QPoint getPositionForIndex(int index);
};

#endif // MOTORIZEDCAPTUREWINDOW_H 