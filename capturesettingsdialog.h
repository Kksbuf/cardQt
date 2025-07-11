#ifndef CAPTURESETTINGSDIALOG_H
#define CAPTURESETTINGSDIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVector>

class CaptureSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CaptureSettingsDialog(QWidget *parent = nullptr);
    QVector<int> getCaptureSequence() const;
    int getImagesInX() const;
    int getImagesInY() const;

private slots:
    void onGridSizeChanged();
    void validateSequence();
    void updateSequenceValidity();

private:
    void setupUI();
    void generateDefaultSequence();
    bool isSequenceValid() const;
    
    QSpinBox *imagesInXSpinBox;
    QSpinBox *imagesInYSpinBox;
    QVector<QLineEdit*> sequenceInputs;
    QLabel *validationLabel;
    QPushButton *okButton;
    QPushButton *cancelButton;
};

#endif // CAPTURESETTINGSDIALOG_H 