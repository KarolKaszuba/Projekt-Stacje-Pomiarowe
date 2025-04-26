QT  += core gui widgets network charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    #apiManager.cpp \
    historymanager.cpp \
    main.cpp \
    mainwindow.cpp \
    window_2_data_vis.cpp


HEADERS += \
    #apiManager.h \
    historymanager.h \
    mainwindow.h \
    window_2_data_vis.h


FORMS += \
    mainwindow.ui \
    window_2_data_vis.ui

TRANSLATIONS += \
    JPO_projekt_2_pl_PL.ts

RESOURCES += resources.qrc


CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

