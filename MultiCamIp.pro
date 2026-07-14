QT  += core gui widgets httpserver network concurrent multimedia multimediawidgets
CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    findnewcamera.cpp \
    httpserwer.cpp \
    main.cpp \
    mainwindow.cpp \
    mediamtxmanager.cpp

HEADERS += \
    findnewcamera.h \
    httpserwer.h \
    mainwindow.h \
    mediamtxmanager.h

#Dynamiczne opencv2
unix:!macx: LIBS += -L/usr/local/lib -lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_imgcodecs -lopencv_calib3d
unix:!macx: LIBS += -L/usr/local/lib -lopencv_features2d -lopencv_flann -lopencv_gapi -lopencv_ml -lopencv_objdetect -lopencv_video -lopencv_videoio
INCLUDEPATH += /usr/local/include/opencv4
DEPENDPATH += /usr/local/include/opencv4

LIBS += -larchive

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
