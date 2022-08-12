
QT += core gui widgets
QT += 3dcore 3drender 3dinput 3dextras
LIBS += -lz

mac:lessThan(QT_MAJOR_VERSION, 6) {
    CONFIG += sdk_no_version_check
}

!win32-msvc* {
    QMAKE_CFLAGS += -std=gnu99
    QMAKE_CXXFLAGS_WARN_ON += -Wno-missing-braces -Wno-missing-field-initializers -Wno-unused-parameter
    QMAKE_CFLAGS_WARN_ON += -Wno-missing-field-initializers -Wno-unused-parameter
    CONFIG += warn_on
}
else {
    # "-wdXXXX" means Warning Disable, "-w1XXXX" means enable for level >= W1
    # 4100 = Unused parameter
    # 4267 = Truncation of size_t
    # 4244, 4305 = Truncation of double to float
    # 4701, 4703 = Use of uninitialized variable
    QMAKE_CXXFLAGS_WARN_ON = -W3 -wd4100 -wd4267 -wd4305 -w14701 -w14703
    QMAKE_CFLAGS_WARN_ON = -W3 -wd4244 -wd4305 -w14701 -w14703
    CONFIG += warn_on
}

QMAKE_CXXFLAGS += -O2
QMAKE_CFLAGS += -O2

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    arcball.cpp \
    calculation_util.cpp \
    configurecalculationdialog.cpp \
    cubefile.cpp \
    cvprojfile.cpp \
    element.cpp \
    elementselector.cpp \
    filehandlers.cpp \
    jsonquery.cpp \
    linebuffer.cpp \
    main.cpp \
    mainwindow.cpp \
    mol3dview.cpp \
    mol3dview/atomentity.cpp \
    mol3dview/atomlabelentity.cpp \
    mol3dview/billboardentity.cpp \
    mol3dview/bondentity.cpp \
    mol3dview/bondhighlightentity.cpp \
    mol3dview/isosurfaceentity.cpp \
    mol3dview/selectionhighlightentity.cpp \
    moldocument.cpp \
    molstruct.cpp \
    molstructgraph.cpp \
    nwchemconfiguration.cpp \
    optimizer.cpp \
    optimizerbabelff.cpp \
    optimizererrordialog.cpp \
    optimizernwchem.cpp \
    optimizerprogressdialog.cpp \
    nwchem.cpp \
    parsehelpers.cpp \
    preferenceswindow.cpp \
    propertieswindow.cpp \
    systempaths.cpp \
    toolbaraddwidget.cpp \
    toolbarmeasurewidget.cpp \
    toolbarpropswidget.cpp \
    unitslineedit.cpp \
    vector3d.cpp \
    volumedata.cpp

HEADERS += \
    arcball.h \
    calculation_util.h \
    configurecalculationdialog.h \
    cubefile.h \
    cvprojfile.h \
    element.h \
    elementselector.h \
    filehandlers.h \
    jsonquery.h \
    linebuffer.h \
    mainwindow.h \
    mol3dview.h \
    mol3dview/atomentity.h \
    mol3dview/atomlabelentity.h \
    mol3dview/billboardentity.h \
    mol3dview/bondentity.h \
    mol3dview/bondhighlightentity.h \
    mol3dview/isosurfaceentity.h \
    mol3dview/selectionhighlightentity.h \
    moldocument.h \
    molstruct.h \
    molstructgraph.h \
    nwchemconfiguration.h \
    optimizer.h \
    optimizerbabelff.h \
    optimizererrordialog.h \
    optimizernwchem.h \
    optimizerprogressdialog.h \
    nwchem.h \
    parsehelpers.h \
    preferenceswindow.h \
    propertieswindow.h \
    systempaths.h \
    toolbaraddwidget.h \
    toolbarmeasurewidget.h \
    toolbarpropswidget.h \
    unitslineedit.h \
    vector3d.h \
    volumedata.h

FORMS += \
    configurecalculationdialog.ui \
    mainwindow.ui \
    optimizererrordialog.ui \
    preferenceswindow.ui \
    propertieswindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

mac {
  ICON = acetic_icon.icns
}

RESOURCES += \
    resources/Resources.qrc

include(3rd-party/qzip/qzip.pri)

DISTFILES += \
    resources/shaders/gl3/bond_hightlight.frag \
    resources/shaders/gl3/bond_hightlight.vert \
    resources/shaders/gl3/selection_hightlight.frag \
    resources/shaders/gl3/selection_hightlight.vert

unix:!mac {
DESTDIR = bin

# There doesn't appear to be a sane way to keep data files in sync without requiring "make install", so
# we just use rsync for now. https://stackoverflow.com/questions/18488154/how-to-get-qmake-to-copy-large-data-files-only-if-they-are-updated
COPYFILES_PREFIX=/share/$${TARGET}
copyfiles.commands += mkdir -p $${OUT_PWD}/$${COPYFILES_PREFIX}; rsync -aru $${PWD}/scripts $${OUT_PWD}/$${COPYFILES_PREFIX}
first.depends = $(first) copyfiles
QMAKE_EXTRA_TARGETS += first copyfiles

DEFINES += DATA_PREFIX='"\\\"$${COPYFILES_PREFIX}/\\\""'
}
mac {
PREFIX_FILES.files = macos/Resources/arm macos/Resources/x86
PREFIX_FILES.path = Contents/Resources/
SHARE_FILES.files = scripts macos/Resources/share/nwchem macos/Resources/share/openbabel macos/Resources/share/openmpi macos/Resources/share/pmix
SHARE_FILES.path = Contents/Resources/share/
QMAKE_BUNDLE_DATA += SHARE_FILES PREFIX_FILES

greaterThan(QT_MAJOR_VERSION, 5) {
    QMAKE_APPLE_DEVICE_ARCHS = x86_64 arm64
} else {
    QMAKE_APPLE_DEVICE_ARCHS = x86_64
}
}
