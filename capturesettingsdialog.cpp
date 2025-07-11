#include "capturesettingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QSet>

CaptureSettingsDialog::CaptureSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
}

void CaptureSettingsDialog::setupUI()
{
    setWindowTitle("Capture Grid Settings");
    setMinimumWidth(400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Grid Settings Group
    QGroupBox *gridGroup = new QGroupBox("Capture Grid Settings");
    QGridLayout *gridLayout = new QGridLayout;

    // Images in X
    gridLayout->addWidget(new QLabel("Images in X:"), 0, 0);
    imagesInXSpinBox = new QSpinBox;
    imagesInXSpinBox->setRange(1, 10);
    imagesInXSpinBox->setValue(3);
    gridLayout->addWidget(imagesInXSpinBox, 0, 1);

    // Images in Y
    gridLayout->addWidget(new QLabel("Images in Y:"), 1, 0);
    imagesInYSpinBox = new QSpinBox;
    imagesInYSpinBox->setRange(1, 10);
    imagesInYSpinBox->setValue(3);
    gridLayout->addWidget(imagesInYSpinBox, 1, 1);

    gridGroup->setLayout(gridLayout);
    mainLayout->addWidget(gridGroup);

    // Sequence Group
    QGroupBox *sequenceGroup = new QGroupBox("Capture Sequence");
    QVBoxLayout *sequenceLayout = new QVBoxLayout;
    
    sequenceLayout->addWidget(new QLabel("Enter capture sequence numbers (1 to N):"));
    
    // Grid for sequence inputs
    QWidget *gridWidget = new QWidget;
    QGridLayout *sequenceGridLayout = new QGridLayout(gridWidget);
    sequenceGridLayout->setSpacing(5);
    
    // Create initial 3x3 grid
    for(int y = 2; y >= 0; y--) {
        for(int x = 0; x < 3; x++) {
            QLineEdit *input = new QLineEdit;
            input->setFixedWidth(50);
            sequenceInputs.append(input);
            sequenceGridLayout->addWidget(input, 2-y, x);
            connect(input, &QLineEdit::textChanged, this, &CaptureSettingsDialog::validateSequence);
        }
    }
    
    sequenceLayout->addWidget(gridWidget);
    
    // Validation label
    validationLabel = new QLabel;
    validationLabel->setStyleSheet("QLabel { color: green; }");
    sequenceLayout->addWidget(validationLabel);
    
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
    connect(imagesInXSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CaptureSettingsDialog::onGridSizeChanged);
    connect(imagesInYSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &CaptureSettingsDialog::onGridSizeChanged);
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    generateDefaultSequence();
}

void CaptureSettingsDialog::onGridSizeChanged()
{
    // Get the grid layout
    QGridLayout *sequenceGrid = qobject_cast<QGridLayout*>(sequenceInputs[0]->parentWidget()->layout());
    
    // Remove existing widgets
    for(auto input : sequenceInputs) {
        sequenceGrid->removeWidget(input);
        input->deleteLater();
    }
    sequenceInputs.clear();
    
    // Create new grid
    int xCount = imagesInXSpinBox->value();
    int yCount = imagesInYSpinBox->value();
    
    for(int y = yCount - 1; y >= 0; y--) {
        for(int x = 0; x < xCount; x++) {
            QLineEdit *input = new QLineEdit;
            input->setFixedWidth(50);
            sequenceInputs.append(input);
            sequenceGrid->addWidget(input, yCount - 1 - y, x);
            connect(input, &QLineEdit::textChanged, this, &CaptureSettingsDialog::validateSequence);
        }
    }
    
    generateDefaultSequence();
}

void CaptureSettingsDialog::generateDefaultSequence()
{
    int xCount = imagesInXSpinBox->value();
    int yCount = imagesInYSpinBox->value();
    int total = xCount * yCount;
    
    // Create a 2D array to store the sequence
    QVector<QVector<int>> grid(yCount, QVector<int>(xCount));
    
    int num = 1;
    // For each column set (left, middle, right, etc.)
    for(int x = 0; x < xCount; x++) {
        if(x % 2 == 0) {
            // Bottom to top
            for(int y = yCount - 1; y >= 0; y--) {
                grid[y][x] = num++;
            }
        } else {
            // Top to bottom
            for(int y = 0; y < yCount; y++) {
                grid[y][x] = num++;
            }
        }
    }
    
    // Update input fields
    for(int y = 0; y < yCount; y++) {
        for(int x = 0; x < xCount; x++) {
            int index = y * xCount + x;
            sequenceInputs[index]->setText(QString::number(grid[y][x]));
        }
    }
    
    validateSequence();
}

void CaptureSettingsDialog::validateSequence()
{
    updateSequenceValidity();
    okButton->setEnabled(isSequenceValid());
}

bool CaptureSettingsDialog::isSequenceValid() const
{
    QSet<int> numbers;
    int total = imagesInXSpinBox->value() * imagesInYSpinBox->value();
    
    // Check all inputs
    for(auto input : sequenceInputs) {
        bool ok;
        int num = input->text().toInt(&ok);
        
        if(!ok || num < 1 || num > total || numbers.contains(num)) {
            return false;
        }
        numbers.insert(num);
    }
    
    return numbers.size() == total;
}

void CaptureSettingsDialog::updateSequenceValidity()
{
    if(isSequenceValid()) {
        validationLabel->setText("Sequence is valid");
        validationLabel->setStyleSheet("QLabel { color: green; }");
    } else {
        validationLabel->setText("Invalid sequence! Use numbers 1 to N without repeats");
        validationLabel->setStyleSheet("QLabel { color: red; }");
    }
}

QVector<int> CaptureSettingsDialog::getCaptureSequence() const
{
    QVector<int> sequence;
    for(auto input : sequenceInputs) {
        sequence.append(input->text().toInt());
    }
    return sequence;
}

int CaptureSettingsDialog::getImagesInX() const
{
    return imagesInXSpinBox->value();
}

int CaptureSettingsDialog::getImagesInY() const
{
    return imagesInYSpinBox->value();
} 