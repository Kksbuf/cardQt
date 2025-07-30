#ifndef MOTORIZEDCAPTURESETTINGSDIALOG_H
#define MOTORIZEDCAPTURESETTINGSDIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QVector>

class MotorizedCaptureSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MotorizedCaptureSettingsDialog(QWidget *parent = nullptr);
    
    int getImagesInX() const { return 4; }  // Fixed 4x4 grid
    int getImagesInY() const { return 4; }  // Fixed 4x4 grid
    QVector<int> getCaptureSequence() const { return sequence; }
    QString getPaperSize() const { return paperSizeCombo->currentText(); }

private slots:
    void validateSequence();
    void generateDefaultSequence();

private:
    void setupUI();
    
    QComboBox *paperSizeCombo;
    QVector<QLineEdit*> sequenceInputs;
    QLabel *validationLabel;
    QPushButton *okButton;
    QPushButton *cancelButton;
    QVector<int> sequence;
};

#endif // MOTORIZEDCAPTURESETTINGSDIALOG_H 