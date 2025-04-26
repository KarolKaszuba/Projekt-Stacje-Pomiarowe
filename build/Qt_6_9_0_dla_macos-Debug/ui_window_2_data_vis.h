/********************************************************************************
** Form generated from reading UI file 'window_2_data_vis.ui'
**
** Created by: Qt User Interface Compiler version 6.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_WINDOW_2_DATA_VIS_H
#define UI_WINDOW_2_DATA_VIS_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCalendarWidget>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_window_2_data_vis
{
public:
    QGroupBox *grBox_sensors;
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QGroupBox *groupBox_2;
    QVBoxLayout *verticalLayout_2;
    QLabel *label_2;
    QCalendarWidget *calendarWidget;
    QListWidget *listWidget;
    QGroupBox *grBox_charts;
    QHBoxLayout *horizontalLayout;
    QCheckBox *wykr_kolowy;
    QPushButton *pushButton;

    void setupUi(QDialog *window_2_data_vis)
    {
        if (window_2_data_vis->objectName().isEmpty())
            window_2_data_vis->setObjectName("window_2_data_vis");
        window_2_data_vis->resize(719, 909);
        grBox_sensors = new QGroupBox(window_2_data_vis);
        grBox_sensors->setObjectName("grBox_sensors");
        grBox_sensors->setGeometry(QRect(0, 40, 320, 250));
        verticalLayout = new QVBoxLayout(grBox_sensors);
        verticalLayout->setSpacing(6);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(9, 9, 9, 9);
        label = new QLabel(grBox_sensors);
        label->setObjectName("label");
        label->setStyleSheet(QString::fromUtf8("background-color: #4CAF50; color: rgb(0, 0, 0); padding: 10px; border-radius: 5px;"));
        label->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayout->addWidget(label);

        groupBox_2 = new QGroupBox(window_2_data_vis);
        groupBox_2->setObjectName("groupBox_2");
        groupBox_2->setGeometry(QRect(320, 40, 400, 250));
        verticalLayout_2 = new QVBoxLayout(groupBox_2);
        verticalLayout_2->setObjectName("verticalLayout_2");
        label_2 = new QLabel(groupBox_2);
        label_2->setObjectName("label_2");
        label_2->setStyleSheet(QString::fromUtf8("background-color: #4CAF50; color: rgb(0, 0, 0); padding: 10px; border-radius: 5px;"));
        label_2->setAlignment(Qt::AlignmentFlag::AlignCenter);

        verticalLayout_2->addWidget(label_2);

        calendarWidget = new QCalendarWidget(groupBox_2);
        calendarWidget->setObjectName("calendarWidget");

        verticalLayout_2->addWidget(calendarWidget);

        listWidget = new QListWidget(window_2_data_vis);
        listWidget->setObjectName("listWidget");
        listWidget->setGeometry(QRect(0, 330, 720, 591));
        grBox_charts = new QGroupBox(window_2_data_vis);
        grBox_charts->setObjectName("grBox_charts");
        grBox_charts->setGeometry(QRect(0, 290, 720, 41));
        horizontalLayout = new QHBoxLayout(grBox_charts);
        horizontalLayout->setObjectName("horizontalLayout");
        wykr_kolowy = new QCheckBox(grBox_charts);
        wykr_kolowy->setObjectName("wykr_kolowy");

        horizontalLayout->addWidget(wykr_kolowy);

        pushButton = new QPushButton(grBox_charts);
        pushButton->setObjectName("pushButton");
        pushButton->setIconSize(QSize(16, 16));
        pushButton->setCheckable(false);

        horizontalLayout->addWidget(pushButton);


        retranslateUi(window_2_data_vis);

        QMetaObject::connectSlotsByName(window_2_data_vis);
    } // setupUi

    void retranslateUi(QDialog *window_2_data_vis)
    {
        window_2_data_vis->setWindowTitle(QCoreApplication::translate("window_2_data_vis", "Dialog", nullptr));
        grBox_sensors->setTitle(QString());
        label->setText(QCoreApplication::translate("window_2_data_vis", "Wybierz dost\304\231pne stanowiska pomiarowe:", nullptr));
        groupBox_2->setTitle(QString());
        label_2->setText(QCoreApplication::translate("window_2_data_vis", "Wybierz przedzia\305\202 czasowy:", nullptr));
        grBox_charts->setTitle(QString());
        wykr_kolowy->setText(QCoreApplication::translate("window_2_data_vis", "Wykres liniowy", nullptr));
        pushButton->setText(QCoreApplication::translate("window_2_data_vis", "Wy\305\233wietl dane", nullptr));
    } // retranslateUi

};

namespace Ui {
    class window_2_data_vis: public Ui_window_2_data_vis {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_WINDOW_2_DATA_VIS_H
