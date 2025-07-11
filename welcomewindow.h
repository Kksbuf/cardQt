#ifndef WELCOMEWINDOW_H
#define WELCOMEWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include "mainwindow.h"

class WelcomeWindow : public QMainWindow
{
    Q_OBJECT

public:
    WelcomeWindow(QWidget *parent = nullptr);

private slots:
    void onStartNewSession();
    void onOpenExistingSession();

private:
    void setupUI();
    QString createSessionFolder();
    QString getCurrentDateTime();
    
    QPushButton *startNewSessionButton;
    QPushButton *openExistingSessionButton;
    QWidget *centralWidget;
    QVBoxLayout *mainLayout;
};

#endif // WELCOMEWINDOW_H 