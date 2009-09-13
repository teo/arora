/*
 * Copyright 2008 Benjamin C. Meyer <ben@meyerhome.net>
 * Copyright 2008 Jason A. Donenfeld <Jason@zx2c4.com>
 * Copyright 2008 Ariya Hidayat <ariya.hidayat@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

/****************************************************************************
**
** Copyright (C) 2007-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "browsermainwindow.h"

#include "aboutdialog.h"
#include "addbookmarkdialog.h"
#include "autosaver.h"
#include "bookmarksdialog.h"
#include "bookmarksmanager.h"
#include "bookmarksmenu.h"
#include "bookmarksmodel.h"
#include "bookmarkstoolbar.h"
#include "browserapplication.h"
#include "clearprivatedata.h"
#include "downloadmanager.h"
#include "edittoolbar.h"
#include "history.h"
#include "languagemanager.h"
#include "networkaccessmanager.h"
#include "settings.h"
#include "sourceviewer.h"
#include "tabbar.h"
#include "tabwidget.h"
#include "toolbardialog.h"
#include "toolbarsearch.h"
#include "webview.h"
#include "webviewsearch.h"

#include <qdesktopwidget.h>
#include <qevent.h>
#include <qfiledialog.h>
#include <qprintdialog.h>
#include <qprintpreviewdialog.h>
#include <qprinter.h>
#include <qsettings.h>
#include <qtextcodec.h>
#include <qmenubar.h>
#include <qmessagebox.h>
#include <qpointer.h>
#include <qstatusbar.h>
#include <qinputdialog.h>
#include <qwidgetaction.h>

#include <qwebframe.h>
#include <qwebhistory.h>

#include <qdebug.h>

BrowserMainWindow::BrowserMainWindow(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
    , m_toolbarSearch(0)
    , m_bookmarksToolbar(0)
    , m_contextMenuToolBar(0)
    , m_tabWidget(new TabWidget(this))
    , m_autoSaver(new AutoSaver(this))
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    statusBar()->setSizeGripEnabled(true);
    // fixes https://bugzilla.mozilla.org/show_bug.cgi?id=219070
    // yes, that's a Firefox bug!
    statusBar()->setLayoutDirection(Qt::LeftToRight);
    setupMenu();
    setupToolBars();

    m_filePrivateBrowsingAction->setChecked(BrowserApplication::isPrivate());

    setCentralWidget(m_tabWidget);

    connect(m_tabWidget, SIGNAL(setCurrentTitle(const QString &)),
            this, SLOT(updateWindowTitle(const QString &)));
    connect(m_tabWidget, SIGNAL(showStatusBarMessage(const QString&)),
            statusBar(), SLOT(showMessage(const QString&)));
    connect(m_tabWidget, SIGNAL(linkHovered(const QString&)),
            statusBar(), SLOT(showMessage(const QString&)));
    connect(m_tabWidget, SIGNAL(loadProgress(int)),
            this, SLOT(loadProgress(int)));
    connect(m_tabWidget, SIGNAL(tabsChanged()),
            m_autoSaver, SLOT(changeOccurred()));
    connect(m_tabWidget, SIGNAL(geometryChangeRequested(const QRect &)),
            this, SLOT(geometryChangeRequested(const QRect &)));
    connect(m_tabWidget, SIGNAL(printRequested(QWebFrame *)),
            this, SLOT(printRequested(QWebFrame *)));
    connect(m_tabWidget, SIGNAL(menuBarVisibilityChangeRequested(bool)),
            menuBar(), SLOT(setVisible(bool)));
    connect(m_tabWidget, SIGNAL(statusBarVisibilityChangeRequested(bool)),
            statusBar(), SLOT(setVisible(bool)));
    connect(m_tabWidget, SIGNAL(toolBarVisibilityChangeRequested(bool)),
            this, SLOT(setToolBarsVisible(bool)));
    connect(m_tabWidget, SIGNAL(lastTabClosed()),
            this, SLOT(lastTabClosed()));
    connect(m_tabWidget, SIGNAL(currentChanged(int)),
            this, SLOT(insertToolBarsBelowTabBar(int)));

    updateWindowTitle();
    loadDefaultState();
    m_tabWidget->newTab();
    m_tabWidget->currentLineEdit()->setFocus();

    // Add each item in the menu bar to the main window so
    // if the menu bar is hidden the shortcuts still work.
    QList<QAction*> actions = menuBar()->actions();
    foreach (QAction *action, actions) {
        if (action->menu())
            actions += action->menu()->actions();
        addAction(action);
        m_mainMenu->addAction(action);
    }
#if defined(Q_WS_MAC)
    setWindowIcon(QIcon());
#endif
#if defined(Q_WS_X11)
    setWindowRole(QLatin1String("browser"));
#endif
    retranslate();
}

BrowserMainWindow::~BrowserMainWindow()
{
    m_autoSaver->changeOccurred();
    m_autoSaver->saveIfNeccessary();
}

BrowserMainWindow *BrowserMainWindow::parentWindow(QWidget *widget)
{
    while (widget) {
        if (BrowserMainWindow *parent = qobject_cast<BrowserMainWindow*>(widget))
            return parent;

        widget = widget->parentWidget();
    }

    qWarning() << "BrowserMainWindow::" << __FUNCTION__ << " used with a widget none of whose parents is a main window.";
    return BrowserApplication::instance()->mainWindow();
}

void BrowserMainWindow::loadDefaultState()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("BrowserMainWindow"));
    QByteArray data = settings.value(QLatin1String("defaultState")).toByteArray();
    if (!restoreState(data))
        restoreDefaultToolBars();
}

QSize BrowserMainWindow::sizeHint() const
{
    QRect desktopRect = QApplication::desktop()->screenGeometry();
    QSize size = desktopRect.size() * 0.9;
    return size;
}

void BrowserMainWindow::save()
{
    BrowserApplication::instance()->saveSession();

    QSettings settings;
    settings.beginGroup(QLatin1String("BrowserMainWindow"));
    QByteArray data = saveState(false);
    settings.setValue(QLatin1String("defaultState"), data);
    settings.endGroup();
}

void BrowserMainWindow::saveToolBarState(QDataStream &out) const
{
    out << QMainWindow::saveState();

    out << iconSize().width();
    out << int(toolButtonStyle());

    QList<EditToolBar*> toolBars = findChildren<EditToolBar*>();
    out << toolBars.size();
    foreach (EditToolBar *toolBar, toolBars) {
        out << toolBar->windowTitle();
        out << toolBar->objectName();

        QList<QAction*> actions = toolBar->actions();
        int count = 0;
        foreach (QAction *action, actions)
            if (toolBar->widgetForAction(action))
                ++count;

        out << count;
        foreach (QAction *action, actions) {
            if (QWidget *widget = toolBar->widgetForAction(action)) {
                out << action->objectName();
                QSizePolicy policy = widget->sizePolicy();
                out << uchar(policy.horizontalStretch())
                       << uchar(policy.verticalStretch());
            }
        }
    }

    out << m_toolBarsBelowTabBar.size();
    foreach (QToolBar* toolBar, m_toolBarsBelowTabBar)
        out << toolBar->objectName() << !toolBar->isHidden();
}

static const qint32 BrowserMainWindowMagic = 0xba;

QByteArray BrowserMainWindow::saveState(bool withTabs) const
{
    int version = 5;
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    stream << qint32(BrowserMainWindowMagic);
    stream << qint32(version);

    // save the normal size so exiting fullscreen/maximize will work reasonably
    stream << normalGeometry().size();
    stream << !statusBar()->isHidden();
    if (withTabs)
        stream << tabWidget()->saveState();
    else
        stream << QByteArray();
    stream << m_tabWidget->tabBar()->showTabBarWhenOneTab();

    // version 3
    stream << isMaximized();
    stream << isFullScreen();
    stream << !menuBar()->isHidden();
    stream << m_menuBarVisible;
    stream << m_statusBarVisible;

    // version 4
    saveToolBarState(stream);

    return data;
}

void BrowserMainWindow::restoreToolBarState(QDataStream &in, int version)
{
    QByteArray mainWindowState;
    int iconSize;
    int toolButtonStyle;

    in >> mainWindowState;

    if (!version || version >= 5) {
        in >> iconSize;
        in >> toolButtonStyle;

        setIconSize(QSize(iconSize, iconSize));
        setToolButtonStyle(Qt::ToolButtonStyle(toolButtonStyle));

        int barDataCount;
        in >> barDataCount;

        while (--barDataCount >= 0) {
            QString toolBarName;
            QString toolBarObjectName;
            int actionCount;

            in >> toolBarName;
            in >> toolBarObjectName;
            in >> actionCount;

            QToolBar *toolBar = addToolBar(toolBarName, toolBarObjectName);
            while (--actionCount >= 0) {
                QString actionName;
                uchar horizontalStretch;
                uchar verticalStretch;

                in >> actionName;
                in >> horizontalStretch;
                in >> verticalStretch;

                if (QAction *action = m_toolBarActions.value(actionName)) {
                    toolBar->addAction(action);
                    if (horizontalStretch || verticalStretch) {
                        QWidget *widget = toolBar->widgetForAction(action);

                        QSizePolicy policy = widget->sizePolicy();
                        policy.setHorizontalStretch(horizontalStretch);
                        policy.setVerticalStretch(verticalStretch);
                        widget->setSizePolicy(policy);
                    }
                }
            }
        }
    }

    QMainWindow::restoreState(mainWindowState);

    QList<QToolBar*> toolBars = findChildren<QToolBar*>();
    int count;
    in >> count;
    while (--count >= 0) {
        QString toolBarName;
        bool visible;
        in >> toolBarName;
        in >> visible;
        foreach (QToolBar *toolBar, toolBars) {
            if (toolBar->objectName() == toolBarName) {
                m_toolBarsBelowTabBar << toolBar;
                toolBar->setVisible(visible);
                break;
            }
        }
    }
    insertToolBarsBelowTabBar(m_tabWidget->currentIndex());

    updateToolBarsVisible();
}

bool BrowserMainWindow::restoreState(const QByteArray &state)
{
    QByteArray sd = state;
    QDataStream stream(&sd, QIODevice::ReadOnly);
    if (stream.atEnd())
        return false;

    qint32 marker;
    qint32 version;
    stream >> marker;
    stream >> version;
    if (marker != BrowserMainWindowMagic || version < 2 || version > 5)
        return false;

    QSize size;
    bool showStatusbar;
    QByteArray tabState;
    bool showTabBarWhenOneTab;
    bool maximized;
    bool fullScreen;
    bool showMenuBar;

    stream >> size;

    if (version < 4) {
        bool showToolbar;
        stream >> showToolbar;
        stream >> showToolbar;
    }

    stream >> showStatusbar;
    stream >> tabState;

    if (version < 5) {
        QByteArray splitterState;
        stream >> splitterState;
    }

    stream >> showTabBarWhenOneTab;

    if (version < 4) {
        qint32 navigationBarLocation;
        stream >> navigationBarLocation;
        stream >> navigationBarLocation;
    }

    if (version >= 3) {
        stream >> maximized;
        stream >> fullScreen;
        stream >> showMenuBar;
        stream >> m_menuBarVisible;
        stream >> m_statusBarVisible;
    } else {
        maximized = false;
        fullScreen = false;
        showMenuBar = true;
        m_menuBarVisible = true;
        m_statusBarVisible = showStatusbar;
    }

    if (size.isValid())
        resize(size);

    if (maximized)
        setWindowState(windowState() | Qt::WindowMaximized);
    if (fullScreen) {
        setWindowState(windowState() | Qt::WindowFullScreen);
        m_viewFullScreenAction->setChecked(true);
    }

    menuBar()->setVisible(showMenuBar);

    statusBar()->setVisible(showStatusbar);

    if (!tabState.isEmpty() && !tabWidget()->restoreState(tabState))
        return false;

    m_tabWidget->tabBar()->setShowTabBarWhenOneTab(showTabBarWhenOneTab);

    if (version >= 4)
        restoreToolBarState(stream, version);
    if (version < 5)
        restoreDefaultToolBars();

    return true;
}

void BrowserMainWindow::lastTabClosed()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("tabs"));
    bool quit = settings.value(QLatin1String("quitAsLastTabClosed"), true).toBool();

    if (quit)
        close();
    else
        m_tabWidget->makeNewTab(true);
}

QAction *BrowserMainWindow::showMenuBarAction() const
{
    return m_viewShowMenuBarAction;
}

void BrowserMainWindow::setupMenu()
{
    m_menuBarVisible = true;

    new QShortcut(QKeySequence(Qt::Key_F6), this, SLOT(swapFocus()));

    // File
    m_fileMenu = new QMenu(menuBar());
    menuBar()->addMenu(m_fileMenu);

    m_fileNewWindowAction = new QAction(m_fileMenu);
    m_fileNewWindowAction->setShortcut(QKeySequence::New);
    connect(m_fileNewWindowAction, SIGNAL(triggered()),
            this, SLOT(fileNew()));
    m_fileMenu->addAction(m_fileNewWindowAction);
    m_fileMenu->addAction(m_tabWidget->newTabAction());

    m_fileOpenFileAction = new QAction(m_fileMenu);
    m_fileOpenFileAction->setShortcut(QKeySequence::Open);
    connect(m_fileOpenFileAction, SIGNAL(triggered()),
            this, SLOT(fileOpen()));
    m_fileMenu->addAction(m_fileOpenFileAction);

    m_fileOpenLocationAction = new QAction(m_fileMenu);
    // Add the location bar shortcuts familiar to users from other browsers
    QList<QKeySequence> openLocationShortcuts;
    openLocationShortcuts.append(QKeySequence(Qt::ControlModifier + Qt::Key_L));
    openLocationShortcuts.append(QKeySequence(Qt::AltModifier + Qt::Key_O));
    openLocationShortcuts.append(QKeySequence(Qt::AltModifier + Qt::Key_D));
    m_fileOpenLocationAction->setShortcuts(openLocationShortcuts);
    connect(m_fileOpenLocationAction, SIGNAL(triggered()),
            this, SLOT(selectLineEdit()));
    m_fileMenu->addAction(m_fileOpenLocationAction);

    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_tabWidget->closeTabAction());
    m_fileMenu->addSeparator();

    m_fileSaveAsAction = new QAction(m_fileMenu);
    m_fileSaveAsAction->setShortcut(QKeySequence::Save);
    connect(m_fileSaveAsAction, SIGNAL(triggered()),
            this, SLOT(fileSaveAs()));
    m_fileMenu->addAction(m_fileSaveAsAction);
    m_fileMenu->addSeparator();

    BookmarksManager *bookmarksManager = BrowserApplication::bookmarksManager();
    m_fileImportBookmarksAction = new QAction(m_fileMenu);
    connect(m_fileImportBookmarksAction, SIGNAL(triggered()),
            bookmarksManager, SLOT(importBookmarks()));
    m_fileMenu->addAction(m_fileImportBookmarksAction);
    m_fileExportBookmarksAction = new QAction(m_fileMenu);
    connect(m_fileExportBookmarksAction, SIGNAL(triggered()),
            bookmarksManager, SLOT(exportBookmarks()));
    m_fileMenu->addAction(m_fileExportBookmarksAction);
    m_fileMenu->addSeparator();

    m_filePrintPreviewAction= new QAction(m_fileMenu);
    connect(m_filePrintPreviewAction, SIGNAL(triggered()),
            this, SLOT(filePrintPreview()));
    m_fileMenu->addAction(m_filePrintPreviewAction);

    m_filePrintAction = new QAction(m_fileMenu);
    m_filePrintAction->setShortcut(QKeySequence::Print);
    connect(m_filePrintAction, SIGNAL(triggered()),
            this, SLOT(filePrint()));
    m_fileMenu->addAction(m_filePrintAction);
    m_fileMenu->addSeparator();

    m_filePrivateBrowsingAction = new QAction(m_fileMenu);
    connect(m_filePrivateBrowsingAction, SIGNAL(triggered()),
            this, SLOT(privateBrowsing()));
    m_filePrivateBrowsingAction->setCheckable(true);
    m_fileMenu->addAction(m_filePrivateBrowsingAction);
    m_fileMenu->addSeparator();

    m_fileCloseWindow = new QAction(m_fileMenu);
    connect(m_fileCloseWindow, SIGNAL(triggered()), this, SLOT(close()));
    m_fileCloseWindow->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W));
    m_fileMenu->addAction(m_fileCloseWindow);

    m_fileQuit = new QAction(m_fileMenu);
    int kdeSessionVersion = QString::fromLocal8Bit(qgetenv("KDE_SESSION_VERSION")).toInt();
    if (kdeSessionVersion != 0)
        connect(m_fileQuit, SIGNAL(triggered()), this, SLOT(close()));
    else
        connect(m_fileQuit, SIGNAL(triggered()), BrowserApplication::instance(), SLOT(quitBrowser()));
    m_fileQuit->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    m_fileMenu->addAction(m_fileQuit);

    // Edit
    m_editMenu = new QMenu(menuBar());
    menuBar()->addMenu(m_editMenu);
    m_editUndoAction = new QAction(m_editMenu);
    m_editUndoAction->setShortcuts(QKeySequence::Undo);
    m_tabWidget->addWebAction(m_editUndoAction, QWebPage::Undo);
    m_editMenu->addAction(m_editUndoAction);
    m_editRedoAction = new QAction(m_editMenu);
    m_editRedoAction->setShortcuts(QKeySequence::Redo);
    m_tabWidget->addWebAction(m_editRedoAction, QWebPage::Redo);
    m_editMenu->addAction(m_editRedoAction);
    m_editMenu->addSeparator();
    m_editCutAction = new QAction(m_editMenu);
    m_editCutAction->setShortcuts(QKeySequence::Cut);
    m_tabWidget->addWebAction(m_editCutAction, QWebPage::Cut);
    m_editMenu->addAction(m_editCutAction);
    m_editCopyAction = new QAction(m_editMenu);
    m_editCopyAction->setShortcuts(QKeySequence::Copy);
    m_tabWidget->addWebAction(m_editCopyAction, QWebPage::Copy);
    m_editMenu->addAction(m_editCopyAction);
    m_editPasteAction = new QAction(m_editMenu);
    m_editPasteAction->setShortcuts(QKeySequence::Paste);
    m_tabWidget->addWebAction(m_editPasteAction, QWebPage::Paste);
    m_editMenu->addAction(m_editPasteAction);
    m_editSelectAllAction = new QAction(m_editMenu);
    m_editSelectAllAction->setShortcuts(QKeySequence::SelectAll);
    m_tabWidget->addWebAction(m_editSelectAllAction, QWebPage::SelectAll);
    m_editMenu->addAction(m_editSelectAllAction);
    m_editMenu->addSeparator();

    m_editFindAction = new QAction(m_editMenu);
    m_editFindAction->setShortcuts(QKeySequence::Find);
    connect(m_editFindAction, SIGNAL(triggered()), this, SLOT(editFind()));
    m_editMenu->addAction(m_editFindAction);
    new QShortcut(QKeySequence(Qt::Key_Slash), this, SLOT(editFind()));

    m_editFindNextAction = new QAction(m_editMenu);
    m_editFindNextAction->setShortcuts(QKeySequence::FindNext);
    connect(m_editFindNextAction, SIGNAL(triggered()), this, SLOT(editFindNext()));
    m_editMenu->addAction(m_editFindNextAction);

    m_editFindPreviousAction = new QAction(m_editMenu);
    m_editFindPreviousAction->setShortcuts(QKeySequence::FindPrevious);
    connect(m_editFindPreviousAction, SIGNAL(triggered()), this, SLOT(editFindPrevious()));
    m_editMenu->addAction(m_editFindPreviousAction);

    m_editMenu->addSeparator();
    m_editPreferencesAction = new QAction(m_editMenu);
    connect(m_editPreferencesAction, SIGNAL(triggered()),
            this, SLOT(preferences()));
    m_editMenu->addAction(m_editPreferencesAction);

    // View
    m_viewMenu = new QMenu(menuBar());
    connect(m_viewMenu, SIGNAL(aboutToShow()),
            this, SLOT(aboutToShowViewMenu()));
    menuBar()->addMenu(m_viewMenu);

    m_viewShowMenuBarAction = new QAction(m_viewMenu);
    m_viewShowMenuBarAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_M));
    connect(m_viewShowMenuBarAction, SIGNAL(triggered()), this, SLOT(viewMenuBar()));
    addAction(m_viewShowMenuBarAction);

    m_toolBarMenuAction = new QAction(this);
    m_viewMenu->addAction(m_toolBarMenuAction);

    m_viewToolBarsAction = new QAction(this);
    connect(m_viewToolBarsAction, SIGNAL(triggered()), this, SLOT(viewToolBars()));
    addAction(m_viewToolBarsAction);

    m_viewEditToolBarsAction = new QAction(this);
    connect(m_viewEditToolBarsAction, SIGNAL(triggered()), this, SLOT(editToolBars()));

    QAction *viewTabBarAction = m_tabWidget->tabBar()->viewTabBarAction();
    m_viewMenu->addAction(viewTabBarAction);
    connect(viewTabBarAction, SIGNAL(toggled(bool)),
            m_autoSaver, SLOT(changeOccurred()));

    m_viewStatusbarAction = new QAction(m_viewMenu);
    connect(m_viewStatusbarAction, SIGNAL(triggered()), this, SLOT(viewStatusbar()));
    m_viewMenu->addAction(m_viewStatusbarAction);

    m_viewMenu->addSeparator();

    m_viewStopAction = new QAction(m_viewMenu);
    QList<QKeySequence> shortcuts;
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_Period));
    shortcuts.append(Qt::Key_Escape);
    m_viewStopAction->setShortcuts(shortcuts);
    m_tabWidget->addWebAction(m_viewStopAction, QWebPage::Stop);
    m_viewMenu->addAction(m_viewStopAction);

    m_viewReloadAction = new QAction(m_viewMenu);
    shortcuts.clear();
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_R));
    shortcuts.append(QKeySequence(Qt::Key_F5));
    m_viewReloadAction->setShortcuts(shortcuts);
    m_tabWidget->addWebAction(m_viewReloadAction, QWebPage::Reload);
    m_viewMenu->addAction(m_viewReloadAction);

    m_viewZoomInAction = new QAction(m_viewMenu);
    shortcuts.clear();
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_Plus));
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_Equal));
    m_viewZoomInAction->setShortcuts(shortcuts);
    connect(m_viewZoomInAction, SIGNAL(triggered()),
            this, SLOT(zoomIn()));
    m_viewMenu->addAction(m_viewZoomInAction);

    m_viewZoomNormalAction = new QAction(m_viewMenu);
    m_viewZoomNormalAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    connect(m_viewZoomNormalAction, SIGNAL(triggered()),
            this, SLOT(zoomNormal()));
    m_viewMenu->addAction(m_viewZoomNormalAction);

    m_viewZoomOutAction = new QAction(m_viewMenu);
    shortcuts.clear();
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_Minus));
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_Underscore));
    m_viewZoomOutAction->setShortcuts(shortcuts);
    connect(m_viewZoomOutAction, SIGNAL(triggered()),
            this, SLOT(zoomOut()));
    m_viewMenu->addAction(m_viewZoomOutAction);

    m_viewZoomTextOnlyAction = new QAction(m_viewMenu);
    m_viewZoomTextOnlyAction->setCheckable(true);
    connect(m_viewZoomTextOnlyAction, SIGNAL(toggled(bool)),
            BrowserApplication::instance(), SLOT(setZoomTextOnly(bool)));
    connect(BrowserApplication::instance(), SIGNAL(zoomTextOnlyChanged(bool)),
            this, SLOT(zoomTextOnlyChanged(bool)));
    m_viewMenu->addAction(m_viewZoomTextOnlyAction);

    m_viewFullScreenAction = new QAction(m_viewMenu);
    m_viewFullScreenAction->setShortcut(Qt::Key_F11);
    connect(m_viewFullScreenAction, SIGNAL(triggered(bool)),
            this, SLOT(viewFullScreen(bool)));
    m_viewFullScreenAction->setCheckable(true);
    m_viewMenu->addAction(m_viewFullScreenAction);


    m_viewMenu->addSeparator();

    m_viewSourceAction = new QAction(m_viewMenu);
    connect(m_viewSourceAction, SIGNAL(triggered()),
            this, SLOT(viewPageSource()));
    m_viewMenu->addAction(m_viewSourceAction);

#if QT_VERSION >= 0x040600 || defined(WEBKIT_TRUNK)
    m_viewMenu->addSeparator();

    m_viewTextEncodingAction = new QAction(m_viewMenu);
    m_viewMenu->addAction(m_viewTextEncodingAction);
    m_viewTextEncodingMenu = new QMenu(m_viewMenu);
    m_viewTextEncodingAction->setMenu(m_viewTextEncodingMenu);
    connect(m_viewTextEncodingMenu, SIGNAL(aboutToShow()),
            this, SLOT(aboutToShowTextEncodingMenu()));
    connect(m_viewTextEncodingMenu, SIGNAL(triggered(QAction *)),
            this, SLOT(viewTextEncoding(QAction *)));
#endif

    // History
    m_historyMenu = new HistoryMenu(this);
    connect(m_historyMenu, SIGNAL(openUrl(const QUrl&, const QString&)),
            m_tabWidget, SLOT(loadUrlFromUser(const QUrl&, const QString&)));
    menuBar()->addMenu(m_historyMenu);
    QList<QAction*> historyActions;

    m_historyBackAction = new QAction(this);
    m_tabWidget->addWebAction(m_historyBackAction, QWebPage::Back);
    m_historyBackAction->setShortcuts(QKeySequence::Back);
    m_historyBackAction->setIconVisibleInMenu(false);

    m_historyForwardAction = new QAction(this);
    m_tabWidget->addWebAction(m_historyForwardAction, QWebPage::Forward);
    m_historyForwardAction->setShortcuts(QKeySequence::Forward);
    m_historyForwardAction->setIconVisibleInMenu(false);

    m_historyHomeAction = new QAction(this);
    connect(m_historyHomeAction, SIGNAL(triggered()), this, SLOT(goHome()));
    m_historyHomeAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H));

    m_historyRestoreLastSessionAction = new QAction(this);
    connect(m_historyRestoreLastSessionAction, SIGNAL(triggered()),
            BrowserApplication::instance(), SLOT(restoreLastSession()));
    m_historyRestoreLastSessionAction->setEnabled(BrowserApplication::instance()->canRestoreSession());

    historyActions.append(m_historyBackAction);
    historyActions.append(m_historyForwardAction);
    historyActions.append(m_historyHomeAction);
    historyActions.append(m_tabWidget->recentlyClosedTabsAction());
    historyActions.append(m_historyRestoreLastSessionAction);
    m_historyMenu->setInitialActions(historyActions);

    // Bookmarks
    m_bookmarksMenu = new BookmarksMenuBarMenu(this);
    connect(m_bookmarksMenu, SIGNAL(openUrl(const QUrl&, const QString &)),
            m_tabWidget, SLOT(loadUrlFromUser(const QUrl&, const QString&)));
    connect(m_bookmarksMenu, SIGNAL(openUrl(const QUrl&, TabWidget::OpenUrlIn, const QString&)),
            m_tabWidget, SLOT(loadUrl(const QUrl&, TabWidget::OpenUrlIn, const QString&)));
    menuBar()->addMenu(m_bookmarksMenu);

    m_bookmarksShowAllAction = new QAction(this);
    m_bookmarksShowAllAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_B));
    connect(m_bookmarksShowAllAction, SIGNAL(triggered()),
            this, SLOT(showBookmarksDialog()));

    m_bookmarksAddAction = new QAction(this);
    m_bookmarksAddAction->setIcon(QIcon(QLatin1String(":addbookmark.png")));
    m_bookmarksAddAction->setIconVisibleInMenu(false);
    connect(m_bookmarksAddAction, SIGNAL(triggered()),
            this, SLOT(addBookmark()));
    m_bookmarksAddAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));

    m_bookmarksAddFolderAction = new QAction(this);
    connect(m_bookmarksAddFolderAction, SIGNAL(triggered()),
            this, SLOT(addBookmarkFolder()));

    QList<QAction*> bookmarksActions;
    bookmarksActions.append(m_bookmarksShowAllAction);
    bookmarksActions.append(m_bookmarksAddAction);
    bookmarksActions.append(tabWidget()->bookmarkTabsAction());
    bookmarksActions.append(m_bookmarksAddFolderAction);
    m_bookmarksMenu->setInitialActions(bookmarksActions);

    // Window
    m_windowMenu = new QMenu(menuBar());
    menuBar()->addMenu(m_windowMenu);
    connect(m_windowMenu, SIGNAL(aboutToShow()),
            this, SLOT(aboutToShowWindowMenu()));
    aboutToShowWindowMenu();

    // Tools
    m_toolsMenu = new QMenu(menuBar());
    menuBar()->addMenu(m_toolsMenu);

    m_toolsWebSearchAction = new QAction(m_toolsMenu);
    connect(m_toolsWebSearchAction, SIGNAL(triggered()),
            this, SLOT(webSearch()));
    m_toolsMenu->addAction(m_toolsWebSearchAction);

    m_toolsClearPrivateDataAction = new QAction(m_toolsMenu);
    connect(m_toolsClearPrivateDataAction, SIGNAL(triggered()),
            this, SLOT(clearPrivateData()));
    m_toolsMenu->addAction(m_toolsClearPrivateDataAction);

    m_toolsEnableInspector = new QAction(m_toolsMenu);
    connect(m_toolsEnableInspector, SIGNAL(triggered(bool)),
            this, SLOT(toggleInspector(bool)));
    m_toolsEnableInspector->setCheckable(true);
    QSettings settings;
    settings.beginGroup(QLatin1String("websettings"));
    m_toolsEnableInspector->setChecked(settings.value(QLatin1String("enableInspector"), false).toBool());
    m_toolsMenu->addAction(m_toolsEnableInspector);

    // Help
    m_helpMenu = new QMenu(menuBar());
    menuBar()->addMenu(m_helpMenu);

    m_helpChangeLanguageAction = new QAction(m_helpMenu);
    connect(m_helpChangeLanguageAction, SIGNAL(triggered()),
            BrowserApplication::languageManager(), SLOT(chooseNewLanguage()));
    m_helpMenu->addAction(m_helpChangeLanguageAction);
    m_helpMenu->addSeparator();

    m_helpAboutQtAction = new QAction(m_helpMenu);
    connect(m_helpAboutQtAction, SIGNAL(triggered()),
            qApp, SLOT(aboutQt()));
    m_helpMenu->addAction(m_helpAboutQtAction);

    m_helpAboutApplicationAction = new QAction(m_helpMenu);
    connect(m_helpAboutApplicationAction, SIGNAL(triggered()),
            this, SLOT(aboutApplication()));
    m_helpMenu->addAction(m_helpAboutApplicationAction);
}

void BrowserMainWindow::aboutToShowViewMenu()
{
    QMenu *oldMenu = m_toolBarMenuAction->menu();
    m_toolBarMenuAction->setMenu(createPopupMenu());
    delete oldMenu;

    m_viewStatusbarAction->setText(statusBar()->isVisible() ? tr("Hide Status Bar") : tr("Show Status Bar"));
}

void BrowserMainWindow::aboutToShowTextEncodingMenu()
{
#if QT_VERSION >= 0x040600 || defined(WEBKIT_TRUNK)
    m_viewTextEncodingMenu->clear();

    int currentCodec = -1;
    QStringList codecs;
    QList<int> mibs = QTextCodec::availableMibs();
    foreach (const int &mib, mibs) {
        QString codec = QLatin1String(QTextCodec::codecForMib(mib)->name());
        codecs.append(codec);
    }
    codecs.sort();

    QString defaultTextEncoding = QWebSettings::globalSettings()->defaultTextEncoding();
    currentCodec = codecs.indexOf(defaultTextEncoding);

    QAction *defaultEncoding = m_viewTextEncodingMenu->addAction(tr("Default"));
    defaultEncoding->setData(-1);
    defaultEncoding->setCheckable(true);
    if (currentCodec == -1)
        defaultEncoding->setChecked(true);
    m_viewTextEncodingMenu->addSeparator();

    for (int i = 0; i < codecs.count(); ++i) {
        const QString &codec = codecs.at(i);
        QAction *action = m_viewTextEncodingMenu->addAction(codec);
        action->setData(i);
        action->setCheckable(true);
        if (currentCodec == i)
            action->setChecked(true);
    }
#endif
}

void BrowserMainWindow::viewTextEncoding(QAction *action)
{
    Q_UNUSED(action);
#if QT_VERSION >= 0x040600 || defined(WEBKIT_TRUNK)
    Q_ASSERT(action);
    QList<QByteArray> codecs = QTextCodec::availableCodecs();
    int offset = action->data().toInt();
    if (offset < 0 || offset >= codecs.count())
        QWebSettings::globalSettings()->setDefaultTextEncoding(QString());
    else
        QWebSettings::globalSettings()->setDefaultTextEncoding(QLatin1String(codecs[offset]));
#endif
}

void BrowserMainWindow::retranslate()
{
    m_fileMenu->setTitle(tr("&File"));
    m_fileNewWindowAction->setText(tr("&New Window"));
    m_fileOpenFileAction->setText(tr("&Open File..."));
    m_fileOpenLocationAction->setText(tr("Open &Location..."));
    m_fileSaveAsAction->setText(tr("&Save As..."));
    m_fileImportBookmarksAction->setText(tr("&Import Bookmarks..."));
    m_fileExportBookmarksAction->setText(tr("&Export Bookmarks..."));
    m_filePrintPreviewAction->setText(tr("P&rint Preview..."));
    m_filePrintAction->setText(tr("&Print..."));
    m_filePrivateBrowsingAction->setText(tr("Private &Browsing..."));
    m_fileCloseWindow->setText(tr("Close Window"));
    m_fileQuit->setText(tr("&Quit"));

    m_editMenu->setTitle(tr("&Edit"));
    m_editUndoAction->setText(tr("&Undo"));
    m_editRedoAction->setText(tr("&Redo"));
    m_editCutAction->setText(tr("Cu&t"));
    m_editCopyAction->setText(tr("&Copy"));
    m_editPasteAction->setText(tr("&Paste"));
    m_editSelectAllAction->setText(tr("Select &All"));
    m_editFindAction->setText(tr("&Find"));
    m_editFindNextAction->setText(tr("Find Nex&t"));
    m_editFindPreviousAction->setText(tr("Find P&revious"));
    m_editPreferencesAction->setText(tr("Prefere&nces..."));
    m_editPreferencesAction->setShortcut(tr("Ctrl+,"));

    m_viewMenu->setTitle(tr("&View"));
    m_viewToolBarsAction->setShortcut(tr("Ctrl+|"));
    m_toolBarMenuAction->setText(tr("Toolbars"));
    m_viewEditToolBarsAction->setText(tr("Customize Toolbars..."));
    m_viewStatusbarAction->setShortcut(tr("Ctrl+/"));
    m_viewShowMenuBarAction->setText(tr("Show Menu Bar"));
    m_viewReloadAction->setText(tr("&Reload Page"));
    m_viewStopAction->setText(tr("&Stop"));
    m_viewZoomInAction->setText(tr("Zoom &In"));
    m_viewZoomNormalAction->setText(tr("Zoom &Normal"));
    m_viewZoomOutAction->setText(tr("Zoom &Out"));
    m_viewZoomTextOnlyAction->setText(tr("Zoom &Text Only"));
    m_viewSourceAction->setText(tr("Page S&ource"));
    m_viewSourceAction->setShortcut(tr("Ctrl+Alt+U"));
    m_viewFullScreenAction->setText(tr("&Full Screen"));
#if QT_VERSION >= 0x040600 || defined(WEBKIT_TRUNK)
    m_viewTextEncodingAction->setText(tr("Text Encoding"));
#endif

    m_historyMenu->setTitle(tr("Hi&story"));
    m_historyBackAction->setText(tr("Back"));
    m_historyForwardAction->setText(tr("Forward"));
    m_historyHomeAction->setText(tr("Home"));
    m_historyRestoreLastSessionAction->setText(tr("Restore Last Session"));

    m_bookmarksMenu->setTitle(tr("&Bookmarks"));
    m_bookmarksShowAllAction->setText(tr("Show All Bookmarks..."));
    m_bookmarksAddAction->setText(tr("Add Bookmark..."));
    m_bookmarksAddFolderAction->setText(tr("Add Folder..."));

    m_windowMenu->setTitle(tr("&Window"));

    m_toolsMenu->setTitle(tr("&Tools"));
    m_toolsWebSearchAction->setText(tr("Web &Search"));
    m_toolsWebSearchAction->setShortcut(QKeySequence(tr("Ctrl+K", "Web Search")));
    m_toolsClearPrivateDataAction->setText(tr("&Clear Private Data"));
    m_toolsClearPrivateDataAction->setShortcut(QKeySequence(tr("Ctrl+Shift+Delete", "Clear Private Data")));
    m_toolsEnableInspector->setText(tr("Enable Web &Inspector"));

    m_helpMenu->setTitle(tr("&Help"));
    m_helpChangeLanguageAction->setText(tr("Switch application language "));
    m_helpAboutQtAction->setText(tr("About &Qt"));
    m_helpAboutApplicationAction->setText(tr("About &%1", "About Browser").arg(QApplication::applicationName()));

    // Toolbar
    m_bookmarksToolbar->setWindowTitle(tr("&Bookmarks"));
    m_stopReloadAction->setText(tr("Reload / Stop"));
    m_mainMenuAction->setText(tr("Menu"));
    m_toolsToolMenuAction->setText(tr("&Tools"));
    m_bookmarksToolMenuAction->setText(tr("&Bookmarks"));
    updateStopReloadActionText(false);
    m_showBelowTabBarAction->setText(tr("Show below tab bar"));

    foreach (EditToolBar *toolBar, findChildren<EditToolBar*>())
        if (toolBar->objectName() == QLatin1String("navigation"))
            toolBar->setWindowTitle(tr("Navigation"));
}

void BrowserMainWindow::addToolBar(QToolBar *toolBar)
{
    toolBar->setMovable(m_editingToolBars);
    connect(toolBar->toggleViewAction(), SIGNAL(triggered(bool)),
            this, SLOT(toolBarViewActionTriggered(bool)));
    connect(toolBar, SIGNAL(destroyed(QObject *)),
            this, SLOT(toolBarDestroyed(QObject *)));
    QMainWindow::addToolBar(toolBar);
}

QToolBar *BrowserMainWindow::addToolBar(const QString &name, const QString &objectName)
{
    EditToolBar *toolBar = new EditToolBar(name, this);

    // try to generate a unique object name
    if (objectName.isEmpty())
        toolBar->setObjectName(name + QString::number(time(0), 36));
    else
        toolBar->setObjectName(objectName);

    toolBar->setEditable(m_editingToolBars);

    addToolBarBreak();
    addToolBar(toolBar);

    return toolBar;
}

static QWidgetAction *createAction(QWidget *widget, QObject *parent)
{
    QWidgetAction *action = new QWidgetAction(parent);
    action->setDefaultWidget(widget);
    return action;
}

void BrowserMainWindow::makeToolBarAction(QAction *action)
{
    m_toolBarActions.insert(action->objectName(), action);
}

void BrowserMainWindow::setupToolBars()
{
    m_toolBarsVisible = false;
    m_editingToolBars = false;

    // As of Qt 4.6, unified toolbars have too many limitations to use easily
    //setUnifiedTitleAndToolBarOnMac(true);

    m_historyBackAction->setIcon(style()->standardIcon(QStyle::SP_ArrowBack, 0, this));
    m_historyBackMenu = new QMenu(this);
    m_historyBackAction->setMenu(m_historyBackMenu);
    connect(m_historyBackMenu, SIGNAL(aboutToShow()),
            this, SLOT(aboutToShowBackMenu()));
    connect(m_historyBackMenu, SIGNAL(triggered(QAction *)),
            this, SLOT(openActionUrl(QAction *)));
    m_historyBackAction->setObjectName(QLatin1String("back"));
    makeToolBarAction(m_historyBackAction);

    m_historyForwardAction->setIcon(style()->standardIcon(QStyle::SP_ArrowForward, 0, this));
    m_historyForwardMenu = new QMenu(this);
    connect(m_historyForwardMenu, SIGNAL(aboutToShow()),
            this, SLOT(aboutToShowForwardMenu()));
    connect(m_historyForwardMenu, SIGNAL(triggered(QAction *)),
            this, SLOT(openActionUrl(QAction *)));
    m_historyForwardAction->setMenu(m_historyForwardMenu);
    m_historyForwardAction->setObjectName(QLatin1String("forward"));
    makeToolBarAction(m_historyForwardAction);

    m_stopReloadAction = new QAction(this);
    m_reloadIcon = style()->standardIcon(QStyle::SP_BrowserReload);
    m_stopReloadAction->setIcon(m_reloadIcon);
    m_stopReloadAction->setObjectName(QLatin1String("stopReload"));
    makeToolBarAction(m_stopReloadAction);

    m_mainMenu = new QMenu(this);
    m_mainMenuAction = new QAction(this);
    m_mainMenuAction->setMenu(m_mainMenu);
    m_mainMenuAction->setObjectName(QLatin1String("mainMenu"));
    makeToolBarAction(m_mainMenuAction);

    m_toolsToolMenu = new QMenu(this);
    m_toolsToolMenu->addAction(m_viewSourceAction);
    m_toolsToolMenu->addAction(m_filePrintPreviewAction);
    m_toolsToolMenu->addAction(m_filePrintAction);
    m_toolsToolMenu->addSeparator();
    m_toolsToolMenu->addMenu(m_historyMenu);
    m_toolsToolMenu->addAction(m_toolsClearPrivateDataAction);
    m_toolsToolMenu->addAction(tr("Downloads"), this, SLOT(downloadManager()), QKeySequence(tr("Ctrl+Y", "Download Manager")));
    m_toolsToolMenu->addAction(m_toolsEnableInspector);
    m_toolsToolMenu->addAction(m_filePrivateBrowsingAction);
    m_toolsToolMenu->addSeparator();
    m_toolsToolMenu->addAction(m_editPreferencesAction);
    m_toolsToolMenu->addSeparator();
    m_toolsToolMenu->addAction(m_helpAboutApplicationAction);

    m_toolsToolMenu->setTitle(tr("&Tools"));
    m_toolsToolMenuAction = new QAction(this);
    m_toolsToolMenuAction->setMenu(m_toolsToolMenu);
    m_toolsToolMenuAction->setObjectName(QLatin1String("toolsToolMenu"));
    makeToolBarAction(m_toolsToolMenuAction);

    m_bookmarksToolMenu = new QMenu(this);
    m_bookmarksToolMenu->addActions(m_bookmarksMenu->actions());
    m_bookmarksToolMenu->setTitle(tr("&Bookmarks"));
    m_bookmarksToolMenuAction = new QAction(this);
    m_bookmarksToolMenuAction->setMenu(m_bookmarksToolMenu);
    m_bookmarksToolMenuAction->setObjectName(QLatin1String("bookmarksToolMenu"));
    makeToolBarAction(m_bookmarksToolMenuAction);

    makeToolBarAction(m_editFindAction);

    m_toolbarSearch = new ToolbarSearch(this);
    connect(m_toolbarSearch, SIGNAL(search(const QUrl&)),
            m_tabWidget, SLOT(loadUrl(const QUrl&)));
    m_toolbarSearchAction = createAction(m_toolbarSearch, this);
    m_toolbarSearchAction->setObjectName(QLatin1String("search"));
    makeToolBarAction(m_toolbarSearchAction);

    m_lineEditsAction = createAction(m_tabWidget->lineEditStack(), this);
    m_lineEditsAction->setObjectName(QLatin1String("location"));
    makeToolBarAction(m_lineEditsAction);

    BookmarksModel *bookmarksModel = BrowserApplication::bookmarksManager()->bookmarksModel();
    m_bookmarksToolbar = new BookmarksToolBar(bookmarksModel, this);
    m_bookmarksToolbar->setObjectName(QLatin1String("bookmarks"));
    connect(m_bookmarksToolbar, SIGNAL(openUrl(const QUrl&, const QString&)),
            m_tabWidget, SLOT(loadUrlFromUser(const QUrl&, const QString&)));
    connect(m_bookmarksToolbar, SIGNAL(openUrl(const QUrl&, TabWidget::OpenUrlIn, const QString&)),
            m_tabWidget, SLOT(loadUrl(const QUrl&, TabWidget::OpenUrlIn, const QString&)));
    addToolBar(m_bookmarksToolbar);

    m_showBelowTabBarAction = new QAction(this);
    m_showBelowTabBarAction->setCheckable(true);
    connect(m_showBelowTabBarAction, SIGNAL(triggered(bool)),
            this, SLOT(setShowBelowTabBar(bool)));
}

void BrowserMainWindow::restoreDefaultToolBars()
{
    foreach (EditToolBar *toolBar, findChildren<EditToolBar*>())
        delete toolBar;

#if defined(Q_WS_MAC)
    setIconSize(QSize(18, 18));
#else
    setIconSize(QSize(24, 24));
#endif
    setToolButtonStyle(Qt::ToolButtonIconOnly);

    QToolBar *navigation = addToolBar(tr("Navigation"), QLatin1String("navigation"));
    navigation->addAction(m_historyBackAction);
    navigation->addAction(m_historyForwardAction);
    navigation->addAction(m_stopReloadAction);
    navigation->addAction(m_lineEditsAction);
    navigation->addAction(m_toolbarSearchAction);

    QWidget *widget = navigation->widgetForAction(m_lineEditsAction);
    QSizePolicy policy = widget->sizePolicy();
    policy.setHorizontalStretch(4);
    widget->setSizePolicy(policy);
    widget = navigation->widgetForAction(m_toolbarSearchAction);
    policy = widget->sizePolicy();
    policy.setHorizontalStretch(1);
    widget->setSizePolicy(policy);

    addToolBarBreak();

    addToolBar(m_bookmarksToolbar);
    if (m_editingToolBars) {
        m_bookmarksToolbar->show();
        m_toolBarsToHide << m_bookmarksToolbar;
    } else {
        m_bookmarksToolbar->hide();
    }
}

void BrowserMainWindow::showBookmarksDialog()
{
    BookmarksDialog *dialog = new BookmarksDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, SIGNAL(openUrl(const QUrl&, TabWidget::OpenUrlIn, const QString &)),
            m_tabWidget, SLOT(loadUrl(const QUrl&, TabWidget::OpenUrlIn, const QString &)));
    dialog->show();
}

void BrowserMainWindow::addBookmark()
{
    WebView *webView = currentTab();
    QString url = QLatin1String(webView->url().toEncoded());
    QString title = webView->title();

    AddBookmarkDialog dialog;
    dialog.setUrl(url);
    dialog.setTitle(title);
    BookmarkNode *menu = BrowserApplication::bookmarksManager()->menu();
    QModelIndex index = BrowserApplication::bookmarksManager()->bookmarksModel()->index(menu);
    dialog.setCurrentIndex(index);
    dialog.exec();
}

void BrowserMainWindow::addBookmarkFolder()
{
    AddBookmarkDialog dialog;
    BookmarksManager *bookmarksManager = BrowserApplication::bookmarksManager();
    BookmarkNode *menu = bookmarksManager->menu();
    QModelIndex index = bookmarksManager->bookmarksModel()->index(menu);
    dialog.setCurrentIndex(index);
    dialog.setFolder(true);
    dialog.exec();
}

void BrowserMainWindow::viewMenuBar()
{
    menuBar()->setVisible(!menuBar()->isVisible());

    m_menuBarVisible = menuBar()->isVisible();
    m_autoSaver->changeOccurred();
}

void BrowserMainWindow::viewStatusbar()
{
    if (statusBar()->isVisible()) {
        statusBar()->close();
    } else {
        statusBar()->show();
    }

    m_statusBarVisible = statusBar()->isVisible();

    m_autoSaver->changeOccurred();
}

void BrowserMainWindow::downloadManager()
{
    BrowserApplication::downloadManager()->show();
}

void BrowserMainWindow::showAndFocus(QWidget *widget)
{
    widget->setFocus();

    if (widget->isVisible())
        return;

    QWidget *widgetToShow = widget;
    QObject *parent = widget;
    while ((parent = parent->parent())) {
        if (QToolBar *toolBar = qobject_cast<QToolBar*>(parent)) {
            widgetToShow = toolBar;
            break;
        }
    }

    if (!widgetToShow)
        return;

    m_shownWidget = widgetToShow;
    widgetToShow->show();

    connect(qApp, SIGNAL(focusChanged(QWidget *, QWidget *)),
            this, SLOT(onFocusChange(QWidget *, QWidget *)));
}

void BrowserMainWindow::onFocusChange(QWidget *oldWidget, QWidget *newWidget)
{
    Q_UNUSED(oldWidget);
    if (!m_shownWidget->isAncestorOf(newWidget)) {
        m_shownWidget->hide();
        disconnect(qApp, SIGNAL(focusChanged(QWidget *, QWidget *)),
                   this, SLOT(onFocusChange(QWidget *, QWidget *)));
    }
}

void BrowserMainWindow::selectLineEdit()
{
    m_tabWidget->currentLineEdit()->selectAll();
    showAndFocus(m_tabWidget->currentLineEdit());
}

void BrowserMainWindow::fileSaveAs()
{
    BrowserApplication::downloadManager()->download(currentTab()->url(), true);
}

void BrowserMainWindow::preferences()
{
    SettingsDialog settingsDialog(this);
    settingsDialog.exec();
}

void BrowserMainWindow::updateStatusbar(const QString &string)
{
    statusBar()->showMessage(string, 2000);
}

void BrowserMainWindow::updateWindowTitle(const QString &title)
{
    if (title.isEmpty()) {
        setWindowTitle(QApplication::applicationName());
    } else {
#if defined(Q_WS_MAC)
        setWindowTitle(title);
#else
        setWindowTitle(tr("%1 - Arora", "Page title and Browser name").arg(title));
#endif
    }
}

void BrowserMainWindow::aboutApplication()
{
    AboutDialog *aboutDialog = new AboutDialog(this);
    aboutDialog->setAttribute(Qt::WA_DeleteOnClose);
    aboutDialog->show();
}

void BrowserMainWindow::fileNew()
{
    BrowserMainWindow *window = BrowserApplication::instance()->newMainWindow();

    QSettings settings;
    settings.beginGroup(QLatin1String("MainWindow"));
    int startup = settings.value(QLatin1String("startupBehavior")).toInt();

    if (startup == 0)
        window->goHome();
}

void BrowserMainWindow::fileOpen()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Open Web Resource"), QString(),
                   tr("Web Resources (*.html *.htm *.svg *.png *.gif *.svgz);;All files (*.*)"));

    if (file.isEmpty())
        return;

    tabWidget()->loadUrl(QUrl::fromLocalFile(file));
}

void BrowserMainWindow::filePrintPreview()
{
    if (!currentTab())
        return;
    QPrintPreviewDialog dialog(this);
    connect(&dialog, SIGNAL(paintRequested(QPrinter *)),
            currentTab(), SLOT(print(QPrinter *)));
    dialog.exec();
}

void BrowserMainWindow::filePrint()
{
    if (!currentTab())
        return;
    printRequested(currentTab()->page()->mainFrame());
}

void BrowserMainWindow::printRequested(QWebFrame *frame)
{
    QPrinter printer;
    QPrintDialog dialog(&printer, this);
    dialog.setWindowTitle(tr("Print Document"));
    if (dialog.exec() != QDialog::Accepted)
        return;
    frame->print(&printer);
}

void BrowserMainWindow::privateBrowsing()
{
    if (!BrowserApplication::isPrivate()) {
        QString title = tr("Are you sure you want to turn on private browsing?");
        QString text1 = tr("When private browsing is turned on, some actions concerning your privacy will be disabled:");

        QStringList actions;
        actions.append(tr("Webpages are not added to the history."));
        actions.append(tr("Items are automatically removed from the Downloads window."));
        actions.append(tr("New cookies are not stored, current cookies can't be accessed."));
        actions.append(tr("Site icons won't be stored."));
        actions.append(tr("Session won't be saved."));
        actions.append(tr("Searches are not added to the pop-up menu in the search box."));
        actions.append(tr("No new network cache is written to disk."));

        QString text2 = tr("Until you close the window, you can still click the Back and Forward "
                           "buttons to return to the webpages you have opened.");

        QString message = QString(QLatin1String("<b>%1</b><p>%2</p><ul><li>%3</li></ul><p>%4</p>"))
                          .arg(title, text1, actions.join(QLatin1String("</li><li>")), text2);

        QMessageBox::StandardButton button = QMessageBox::question(this, tr("Private Browsing"), message,
                                                                   QMessageBox::Ok | QMessageBox::Cancel,
                                                                   QMessageBox::Ok);
        if (button == QMessageBox::Ok)
            BrowserApplication::setPrivate(true);
        else
            m_filePrivateBrowsingAction->setChecked(false);
    } else {
        BrowserApplication::setPrivate(false);
    }
}

void BrowserMainWindow::zoomTextOnlyChanged(bool textOnly)
{
    m_viewZoomTextOnlyAction->setChecked(textOnly);
}

void BrowserMainWindow::privacyChanged(bool isPrivate)
{
    m_filePrivateBrowsingAction->setChecked(isPrivate);
    if (!isPrivate)
        tabWidget()->clear();
}

void BrowserMainWindow::closeEvent(QCloseEvent *event)
{
    if (!BrowserApplication::instance()->allowToCloseWindow(this)) {
        event->ignore();
        if (m_tabWidget->count() == 0)
            m_tabWidget->newTab();
        return;
    }

    if (m_tabWidget->count() > 1) {
        QSettings settings;
        settings.beginGroup(QLatin1String("tabs"));
        bool confirm = settings.value(QLatin1String("confirmClosingMultipleTabs"), true).toBool();
        if (confirm) {
            int ret = QMessageBox::warning(this, QString(),
                                           tr("Are you sure you want to close the window?"
                                              "  There are %1 tabs open").arg(m_tabWidget->count()),
                                           QMessageBox::Yes | QMessageBox::No,
                                           QMessageBox::No);
            if (ret == QMessageBox::No) {
                event->ignore();
                return;
            }
        }
    }

    event->accept();
}

void BrowserMainWindow::mousePressEvent(QMouseEvent *event)
{
    switch (event->button()) {
    case Qt::XButton1:
        m_historyBackAction->activate(QAction::Trigger);
        break;
    case Qt::XButton2:
        m_historyForwardAction->activate(QAction::Trigger);
        break;
    default:
        QMainWindow::mousePressEvent(event);
        break;
    }
}

void BrowserMainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        retranslate();
    QMainWindow::changeEvent(event);
}

void BrowserMainWindow::editFind()
{
    tabWidget()->webViewSearch(m_tabWidget->currentIndex())->showFind();
}

void BrowserMainWindow::editFindNext()
{
    tabWidget()->webViewSearch(m_tabWidget->currentIndex())->findNext();
}

void BrowserMainWindow::editFindPrevious()
{
    tabWidget()->webViewSearch(m_tabWidget->currentIndex())->findPrevious();
}

void BrowserMainWindow::zoomIn()
{
    if (!currentTab())
        return;
    currentTab()->zoomIn();
}

void BrowserMainWindow::zoomNormal()
{
    if (!currentTab())
        return;
    currentTab()->resetZoom();
}

void BrowserMainWindow::zoomOut()
{
    if (!currentTab())
        return;
    currentTab()->zoomOut();
}

void BrowserMainWindow::viewFullScreen(bool makeFullScreen)
{
    if (makeFullScreen) {
        setWindowState(windowState() | Qt::WindowFullScreen);

        menuBar()->hide();
        statusBar()->hide();
    } else {
        setWindowState(windowState() & ~Qt::WindowFullScreen);

        menuBar()->setVisible(m_menuBarVisible);
        statusBar()->setVisible(m_statusBarVisible);
    }
}

void BrowserMainWindow::viewPageSource()
{
    if (!currentTab())
        return;

    QString title = currentTab()->title();
    QString markup = currentTab()->page()->mainFrame()->toHtml();
    QUrl url = currentTab()->url();
    SourceViewer *viewer = new SourceViewer(markup, title, url, this);
    viewer->setAttribute(Qt::WA_DeleteOnClose);
    viewer->show();
}

void BrowserMainWindow::goHome()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("MainWindow"));
    QString home = settings.value(QLatin1String("home"), QLatin1String("about:home")).toString();
    tabWidget()->loadString(home);
}

void BrowserMainWindow::webSearch()
{
    m_toolbarSearch->selectAll();
    showAndFocus(m_toolbarSearch);
}

void BrowserMainWindow::clearPrivateData()
{
    ClearPrivateData dialog;
    dialog.exec();
}

void BrowserMainWindow::toggleInspector(bool enable)
{
    QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, enable);
    if (enable) {
        int result = QMessageBox::question(this, tr("Web Inspector"),
                                           tr("The web inspector will only work correctly for pages that were loaded after enabling.\n"
                                              "Do you want to reload all pages?"),
                                           QMessageBox::Yes | QMessageBox::No);
        if (result == QMessageBox::Yes) {
            m_tabWidget->reloadAllTabs();
        }
    }
    QSettings settings;
    settings.beginGroup(QLatin1String("websettings"));
    settings.setValue(QLatin1String("enableInspector"), enable);
}

void BrowserMainWindow::swapFocus()
{
    if (currentTab()->hasFocus()) {
        m_tabWidget->currentLineEdit()->setFocus();
        m_tabWidget->currentLineEdit()->selectAll();
    } else {
        currentTab()->setFocus();
    }
}

TabWidget *BrowserMainWindow::tabWidget() const
{
    return m_tabWidget;
}

WebView *BrowserMainWindow::currentTab() const
{
    return m_tabWidget->currentWebView();
}

ToolbarSearch *BrowserMainWindow::toolbarSearch() const
{
    return m_toolbarSearch;
}

void BrowserMainWindow::updateStopReloadActionText(bool loading)
{
    if (loading) {
        m_stopReloadAction->setToolTip(tr("Stop loading the current page"));
        m_stopReloadAction->setIconText(tr("Stop"));
    } else {
        m_stopReloadAction->setToolTip(tr("Reload the current page"));
        m_stopReloadAction->setIconText(tr("Reload"));
    }
}

void BrowserMainWindow::loadProgress(int progress)
{
    if (progress < 100 && progress > 0) {
        disconnect(m_stopReloadAction, SIGNAL(triggered()), m_viewReloadAction, SLOT(trigger()));
        if (m_stopIcon.isNull())
            m_stopIcon = style()->standardIcon(QStyle::SP_BrowserStop);
        m_stopReloadAction->setIcon(m_stopIcon);
        connect(m_stopReloadAction, SIGNAL(triggered()), m_viewStopAction, SLOT(trigger()));
        updateStopReloadActionText(true);
    } else {
        disconnect(m_stopReloadAction, SIGNAL(triggered()), m_viewStopAction, SLOT(trigger()));
        m_stopReloadAction->setIcon(m_reloadIcon);
        connect(m_stopReloadAction, SIGNAL(triggered()), m_viewReloadAction, SLOT(trigger()));
        updateStopReloadActionText(false);
    }
}

void BrowserMainWindow::aboutToShowBackMenu()
{
    m_historyBackMenu->clear();
    if (!currentTab())
        return;
    QWebHistory *history = currentTab()->history();
    int historyCount = history->count();
    for (int i = history->backItems(historyCount).count() - 1; i >= 0; --i) {
        QWebHistoryItem item = history->backItems(history->count()).at(i);
        QAction *action = new QAction(this);
        action->setData(-1*(historyCount - i - 1));
        QIcon icon = BrowserApplication::instance()->icon(item.url());
        action->setIcon(icon);
        action->setText(item.title());
        m_historyBackMenu->addAction(action);
    }
}

void BrowserMainWindow::aboutToShowForwardMenu()
{
    m_historyForwardMenu->clear();
    if (!currentTab())
        return;
    QWebHistory *history = currentTab()->history();
    int historyCount = history->count();
    for (int i = 0; i < history->forwardItems(history->count()).count(); ++i) {
        QWebHistoryItem item = history->forwardItems(historyCount).at(i);
        QAction *action = new QAction(this);
        action->setData(historyCount - i);
        QIcon icon = BrowserApplication::instance()->icon(item.url());
        action->setIcon(icon);
        action->setText(item.title());
        m_historyForwardMenu->addAction(action);
    }
}

void BrowserMainWindow::aboutToShowWindowMenu()
{
    m_windowMenu->clear();
    m_windowMenu->addAction(m_tabWidget->nextTabAction());
    m_windowMenu->addAction(m_tabWidget->previousTabAction());
    m_windowMenu->addSeparator();
    m_windowMenu->addAction(tr("Downloads"), this, SLOT(downloadManager()), QKeySequence(tr("Ctrl+Y", "Download Manager")));

    m_windowMenu->addSeparator();
    QList<BrowserMainWindow*> windows = BrowserApplication::instance()->mainWindows();
    for (int i = 0; i < windows.count(); ++i) {
        BrowserMainWindow *window = windows.at(i);
        QAction *action = m_windowMenu->addAction(window->windowTitle(), this, SLOT(showWindow()));
        action->setData(i);
        action->setCheckable(true);
        if (window == this)
            action->setChecked(true);
    }
}

void BrowserMainWindow::showWindow()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        QVariant v = action->data();
        if (v.canConvert<int>()) {
            int offset = qvariant_cast<int>(v);
            QList<BrowserMainWindow*> windows = BrowserApplication::instance()->mainWindows();
            windows.at(offset)->activateWindow();
            windows.at(offset)->raise();
            windows.at(offset)->currentTab()->setFocus();
        }
    }
}

void BrowserMainWindow::openActionUrl(QAction *action)
{
    int offset = action->data().toInt();
    QWebHistory *history = currentTab()->history();
    if (offset < 0)
        history->goToItem(history->backItems(-1*offset).first()); // back
    else if (offset > 0)
        history->goToItem(history->forwardItems(history->count() - offset + 1).back()); // forward
}

void BrowserMainWindow::geometryChangeRequested(const QRect &geometry)
{
    setGeometry(geometry);
}

QMenu *BrowserMainWindow::createPopupMenu()
{
    QMenu *menu = QMainWindow::createPopupMenu();
    if (!menu)
        menu = new QMenu(this);

    foreach (QToolBar *toolBar, m_toolBarsBelowTabBar)
        menu->addAction(toolBar->toggleViewAction());

    m_viewToolBarsAction->setText(m_toolBarsVisible ? tr("Hide Toolbars") : tr("Show Toolbars"));
    QAction *first = menu->isEmpty() ? 0 : menu->actions().first();
    first = menu->insertSeparator(first);
    menu->insertAction(first, m_viewToolBarsAction);

    if (m_editingToolBars) {
        if (m_contextMenuToolBar) {
            menu->addSeparator();

            m_showBelowTabBarAction->setChecked(m_contextMenuToolBar->parentWidget() != this);
            m_showBelowTabBarAction->setData(quintptr(m_contextMenuToolBar));
            menu->addAction(m_showBelowTabBarAction);

            if (m_contextMenuToolBar->inherits("EditToolBar"))
                menu->addAction(tr("Delete Toolbar"), m_contextMenuToolBar, SLOT(deleteLater()));
        }
    } else {
        menu->addSeparator();
        menu->addAction(m_viewEditToolBarsAction);
    }

    return menu;
}

void BrowserMainWindow::viewToolBars()
{
    setToolBarsVisible(!m_toolBarsVisible);
}

void BrowserMainWindow::setToolBarsVisible(bool visible)
{
    if (visible == m_toolBarsVisible)
        return;

    if (visible) {
        bool anyShown = false;
        foreach (QToolBar *toolBar, findChildren<QToolBar*>()) {
            // only reshow toolbars that were not hidden explicitly
            if (!toolBar->property("bulk_hidden").toBool())
                continue;
            anyShown = true;
            toolBar->show();
        }

        // show all the toolbars if all were hidden explicitly
        if (!anyShown)
            foreach (QToolBar *toolBar, findChildren<QToolBar*>())
                toolBar->show();
    } else {
        foreach (QToolBar *toolBar, findChildren<QToolBar*>()) {
            if (!toolBar->isHidden()) {
                toolBar->setProperty("bulk_hidden", true);
                toolBar->hide();
            }
        }
    }

    m_toolBarsVisible = visible;
}

void BrowserMainWindow::toolBarViewActionTriggered(bool visible)
{
    if (!visible)
        sender()->parent()->setProperty("bulk_hidden", QVariant());
    updateToolBarsVisible();
    m_autoSaver->changeOccurred();
}

void BrowserMainWindow::updateToolBarsVisible()
{
    m_toolBarsVisible = false;
    foreach (QToolBar *toolBar, findChildren<QToolBar*>()) {
        if (!toolBar->isHidden()) {
            m_toolBarsVisible = true;
            return;
        }
    }
}

bool BrowserMainWindow::event(QEvent *event)
{
    // Qt's implementation doesn't actually hide the toolbars; it moves them
    // offscreen. We'll use our own so our menu actions are updated correctly.
    if (event->type() == QEvent::ToolBarChange) {
        viewToolBars();
        return true;
    }
    return QMainWindow::event(event);
}

void BrowserMainWindow::editToolBars()
{
    if (m_editingToolBars)
        return;

    m_savedState.clear();
    QDataStream state(&m_savedState, QIODevice::WriteOnly);
    saveToolBarState(state);

    m_editingToolBars = true;

    foreach (QToolBar *toolBar, findChildren<QToolBar*>()) {
        if (!toolBar->isVisible()) {
            toolBar->show();
            m_toolBarsToHide << toolBar;
        }

        toolBar->setMovable(true);
        if (EditToolBar *edit = qobject_cast<EditToolBar*>(toolBar))
            edit->setEditable(true);
    }

    m_bookmarksToolbar->setContextMenuPolicy(Qt::NoContextMenu);

    ToolBarDialog *dialog = new ToolBarDialog(this);
    connect(dialog, SIGNAL(newToolBarRequested(const QString&)),
            this, SLOT(addToolBar(const QString&)));
    connect(dialog, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)),
            this, SLOT(setToolButtonStyle(Qt::ToolButtonStyle)));
    connect(dialog, SIGNAL(iconSizeChanged(const QSize&)),
            this, SLOT(setIconSize(const QSize&)));
    connect(dialog, SIGNAL(restoreDefaultToolBars()),
            this, SLOT(restoreDefaultToolBars()));
    connect(dialog, SIGNAL(finished(int)),
            this, SLOT(toolBarDialogClosed(int)));

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();

    dialog->addActions(m_toolBarActions.values());
}

void BrowserMainWindow::toolBarDialogClosed(int result)
{
    m_editingToolBars = false;

    if (result == QDialog::Accepted) {
        foreach (QToolBar *toolBar, findChildren<QToolBar*>()) {
            toolBar->setMovable(false);
            if (EditToolBar *edit = qobject_cast<EditToolBar*>(toolBar))
                edit->setEditable(false);
        }
    } else {
        foreach (EditToolBar *toolBar, findChildren<EditToolBar*>())
            delete toolBar;
        m_toolBarsBelowTabBar.clear();
        QDataStream in(&m_savedState, QIODevice::ReadOnly);
        restoreToolBarState(in);
    }

    m_savedState.clear();

    foreach (QPointer<QToolBar> toolBar, m_toolBarsToHide)
        if (toolBar)
            toolBar->hide();
    m_toolBarsToHide.clear();

    m_bookmarksToolbar->setContextMenuPolicy(Qt::CustomContextMenu);

    m_autoSaver->changeOccurred();
}

void BrowserMainWindow::insertToolBarsBelowTabBar(int index)
{
    WebViewWithSearch *webViewSearch = qobject_cast<WebViewWithSearch*>(m_tabWidget->widget(index));
    if (!webViewSearch)
        return;

    foreach (QToolBar *widget, m_toolBarsBelowTabBar)
        webViewSearch->addWidget(widget);
}

void BrowserMainWindow::setShowBelowTabBar(bool below)
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        QVariant v = action->data();
        if (v.canConvert<quintptr>()) {
            QToolBar *toolBar = reinterpret_cast<QToolBar*>(qvariant_cast<quintptr>(v));
            if (below) {
                m_toolBarsBelowTabBar << toolBar;
                toolBar->setOrientation(Qt::Horizontal);
                if (WebViewWithSearch *w = qobject_cast<WebViewWithSearch*>(m_tabWidget->currentWebView()->parent()))
                    w->addWidget(toolBar);
            } else {
                m_toolBarsBelowTabBar.removeOne(toolBar);
                addToolBarBreak();
                addToolBar(toolBar);
            }
        }
    }
}

void BrowserMainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QWidget *child = childAt(event->pos());
    for (; child && child != this; child = child->parentWidget()) {
        if (QToolBar *toolBar = qobject_cast<QToolBar*>(child)) {
            m_contextMenuToolBar = toolBar;
            if (QMenu *menu = createPopupMenu()) {
                menu->exec(event->globalPos());
                delete menu;
            }
            m_contextMenuToolBar = 0;
            return;
        }
    }

    QMainWindow::contextMenuEvent(event);
}

void BrowserMainWindow::toolBarDestroyed(QObject *object)
{
    if (QToolBar *toolBar = reinterpret_cast<QToolBar*>(object))
        m_toolBarsBelowTabBar.removeOne(toolBar);
}

void BrowserMainWindow::setToolButtonStyle(Qt::ToolButtonStyle style)
{
    QMainWindow::setToolButtonStyle(style);
}

void BrowserMainWindow::setIconSize(const QSize &size)
{
    QMainWindow::setIconSize(size);
}
