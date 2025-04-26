/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QGridLayout *gridLayout;
    QLabel *headerLabel;
    QLineEdit *lineEdit_street_town;
    QPushButton *pushButton_szukaj;
    QListWidget *stationList;
    QLineEdit *lineEdit_promien;
    QLabel *statusLabel;
    QPushButton *pushButton_history;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(450, 600);
        QSizePolicy sizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(MainWindow->sizePolicy().hasHeightForWidth());
        MainWindow->setSizePolicy(sizePolicy);
        MainWindow->setMaximumSize(QSize(600, 1000));
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName("centralWidget");
        gridLayout = new QGridLayout(centralWidget);
        gridLayout->setObjectName("gridLayout");
        gridLayout->setContentsMargins(10, 10, 10, 10);
        headerLabel = new QLabel(centralWidget);
        headerLabel->setObjectName("headerLabel");
        QSizePolicy sizePolicy1(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(headerLabel->sizePolicy().hasHeightForWidth());
        headerLabel->setSizePolicy(sizePolicy1);
        headerLabel->setMinimumSize(QSize(0, 50));
        headerLabel->setMaximumSize(QSize(10000, 10000));
        QFont font;
        font.setPointSize(14);
        font.setBold(true);
        headerLabel->setFont(font);
        headerLabel->setStyleSheet(QString::fromUtf8("background-color: #4CAF50; color: rgb(0, 0, 0); padding: 10px; border-radius: 5px;"));
        headerLabel->setAlignment(Qt::AlignmentFlag::AlignCenter);

        gridLayout->addWidget(headerLabel, 0, 0, 1, 2);

        lineEdit_street_town = new QLineEdit(centralWidget);
        lineEdit_street_town->setObjectName("lineEdit_street_town");
        lineEdit_street_town->setAutoFillBackground(false);
        lineEdit_street_town->setStyleSheet(QString::fromUtf8("border-radius: 5px; padding 5px;"));
        lineEdit_street_town->setFrame(true);
        lineEdit_street_town->setClearButtonEnabled(true);

        gridLayout->addWidget(lineEdit_street_town, 1, 0, 1, 1);

        pushButton_szukaj = new QPushButton(centralWidget);
        pushButton_szukaj->setObjectName("pushButton_szukaj");

        gridLayout->addWidget(pushButton_szukaj, 1, 1, 1, 1);

        stationList = new QListWidget(centralWidget);
        stationList->setObjectName("stationList");
        stationList->setStyleSheet(QString::fromUtf8("QListWidget::item { background-color: rgb(26, 165, 108); border-radius: 5px; padding: 10px; margin-bottom: 10px; }\n"
"QListWidget::item:selected { background-color: rgb(128, 127, 129); }"));

        gridLayout->addWidget(stationList, 4, 0, 1, 2);

        lineEdit_promien = new QLineEdit(centralWidget);
        lineEdit_promien->setObjectName("lineEdit_promien");
        QSizePolicy sizePolicy2(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(lineEdit_promien->sizePolicy().hasHeightForWidth());
        lineEdit_promien->setSizePolicy(sizePolicy2);
        lineEdit_promien->setStyleSheet(QString::fromUtf8("border-radius: 5px; padding 5px;"));
        lineEdit_promien->setClearButtonEnabled(true);

        gridLayout->addWidget(lineEdit_promien, 2, 0, 1, 1);

        statusLabel = new QLabel(centralWidget);
        statusLabel->setObjectName("statusLabel");
        statusLabel->setEnabled(true);
        statusLabel->setMaximumSize(QSize(327, 40));
        QFont font1;
        font1.setPointSize(12);
        font1.setKerning(false);
        statusLabel->setFont(font1);
        statusLabel->setStyleSheet(QString::fromUtf8("color rgb(0, 0, 0);"));

        gridLayout->addWidget(statusLabel, 3, 0, 1, 1);

        pushButton_history = new QPushButton(centralWidget);
        pushButton_history->setObjectName("pushButton_history");

        gridLayout->addWidget(pushButton_history, 3, 1, 1, 1);

        MainWindow->setCentralWidget(centralWidget);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "Stacje Pomiarowe", nullptr));
        headerLabel->setText(QCoreApplication::translate("MainWindow", "Projekt Stacje Pomiarowe", nullptr));
        lineEdit_street_town->setText(QString());
        lineEdit_street_town->setPlaceholderText(QCoreApplication::translate("MainWindow", "ulica numer, Miasto lub Miasto", nullptr));
        pushButton_szukaj->setText(QCoreApplication::translate("MainWindow", "Szukaj", nullptr));
        lineEdit_promien->setPlaceholderText(QCoreApplication::translate("MainWindow", "promie\305\204 [km] (opcjonalnie)", nullptr));
        statusLabel->setText(QCoreApplication::translate("MainWindow", "\305\201adowanie danych...", nullptr));
        pushButton_history->setText(QCoreApplication::translate("MainWindow", "HISTORIA", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
