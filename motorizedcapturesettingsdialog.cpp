#include "motorizedcapturesettingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QSet>

MotorizedCaptureSettingsDialog::MotorizedCaptureSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Motorized Capture Settings");
    setupUI();
    generateDefaultSequence();
}

void MotorizedCaptureSettingsDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Paper size selection
    QGroupBox *sizeGroup = new QGroupBox("Paper Size");
    QVBoxLayout *sizeLayout = new QVBoxLayout;
    
    paperSizeCombo = new QComboBox;
    paperSizeCombo->addItem("A4 (297 × 210 mm)");
    paperSizeCombo->addItem("A5 (210 × 148 mm)");
    sizeLayout->addWidget(paperSizeCombo);
    
    sizeGroup->setLayout(sizeLayout);
    mainLayout->addWidget(sizeGroup);

    // Sequence input
    QGroupBox *sequenceGroup = new QGroupBox("Capture Sequence (4×4)");
    QGridLayout *sequenceLayout = new QGridLayout;
    sequenceLayout->setSpacing(5);

    // Create 4x4 grid of input boxes
    for(int y = 3; y >= 0; y--) {
        for(int x = 0; x < 4; x++) {
            QLineEdit *input = new QLineEdit;
            input->setFixedWidth(50);
            input->setAlignment(Qt::AlignCenter);
            sequenceInputs.append(input);
            sequenceLayout->addWidget(input, 3-y, x);
            connect(input, &QLineEdit::textChanged, this, &MotorizedCaptureSettingsDialog::validateSequence);
        }
    }
    
    // Validation label
    validationLabel = new QLabel;
    validationLabel->setStyleSheet("QLabel { color: green; }");
    sequenceLayout->addWidget(validationLabel, 4, 0, 1, 4);
    
    sequenceGroup->setLayout(sequenceLayout);
    mainLayout->addWidget(sequenceGroup);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    okButton = new QPushButton("Start Capture");
    cancelButton = new QPushButton("Cancel");
    
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    // Connect signals
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void MotorizedCaptureSettingsDialog::generateDefaultSequence()
{
    // Default 4x4 sequence:
    // 4  5  12 13
    // 3  6  11 14
    // 2  7  10 15
    // 1  8  9  16
    QVector<int> defaultSequence = {
        4, 5, 12, 13,
        3, 6, 11, 14,
        2, 7, 10, 15,
        1, 8, 9, 16
    };

    // Update input fields
    for(int i = 0; i < sequenceInputs.size(); i++) {
        sequenceInputs[i]->setText(QString::number(defaultSequence[i]));
    }

    sequence = defaultSequence;
    validationLabel->setText("Valid sequence");
    okButton->setEnabled(true);
}

void MotorizedCaptureSettingsDialog::validateSequence()
{
    QVector<int> numbers;
    bool valid = true;
    QString errorMsg;

    // Collect all numbers
    for(QLineEdit *input : sequenceInputs) {
        bool ok;
        int num = input->text().toInt(&ok);
        if(!ok || num < 1 || num > 16) {
            valid = false;
            errorMsg = "Numbers must be between 1 and 16";
            break;
        }
        numbers.append(num);
    }

    // Check for duplicates
    if(valid) {
        QSet<int> uniqueNumbers(numbers.begin(), numbers.end());
        if(uniqueNumbers.size() != 16) {
            valid = false;
            errorMsg = "Each number must be used exactly once";
        }
    }

    // Update UI based on validation
    if(valid) {
        validationLabel->setText("Valid sequence");
        validationLabel->setStyleSheet("QLabel { color: green; }");
        sequence = numbers;
        okButton->setEnabled(true);
    } else {
        validationLabel->setText(errorMsg);
        validationLabel->setStyleSheet("QLabel { color: red; }");
        okButton->setEnabled(false);
    }
} 