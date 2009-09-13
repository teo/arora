/*
 * Copyright 2008 Benjamin C. Meyer <ben@meyerhome.net>
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

#ifndef BROWSERMAINWINDOW_H
#define BROWSERMAINWINDOW_H

#include <qmainwindow.h>
#include <qhash.h>
#include <qicon.h>
#include <qurl.h>

class AutoSaver;
class BookmarksToolBar;
class QWebFrame;
class TabWidget;
class ToolbarSearch;
class WebView;
class QFrame;
class HistoryMenu;
class BookmarksMenuBarMenu;
template<class T> class QPointer;


/*!
    The MainWindow of the Browser Application.

    Handles the tab widget and all the actions
 */
class BrowserMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    BrowserMainWindow(QWidget *parent = 0, Qt::WindowFlags flags = 0);
    ~BrowserMainWindow();
    QSize sizeHint() const;

public:
    static BrowserMainWindow *parentWindow(QWidget *widget);

    TabWidget *tabWidget() const;
    WebView *currentTab() const;
    ToolbarSearch *toolbarSearch() const;
    QAction *showMenuBarAction() const;

    void saveToolBarState(QDataStream &) const;
    QByteArray saveState(bool withTabs = true) const;
    void restoreToolBarState(QDataStream &, int version = 0);
    bool restoreState(const QByteArray &state);

    QMenu *createPopupMenu();
    void addToolBar(QToolBar *);

public slots:
    void goHome();
    void privacyChanged(bool isPrivate);
    void zoomTextOnlyChanged(bool textOnly);
    QToolBar *addToolBar(const QString &name, const QString &objectName = QString());

    void setToolButtonStyle(Qt::ToolButtonStyle);
    void setIconSize(const QSize &);

protected:
    void closeEvent(QCloseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void changeEvent(QEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
    bool event(QEvent *event);

private slots:
    void save();

    void lastTabClosed();

    void loadProgress(int);
    void updateStatusbar(const QString &string);
    void updateWindowTitle(const QString &title = QString());

    void preferences();

    void fileNew();
    void fileOpen();
    void filePrintPreview();
    void filePrint();
    void privateBrowsing();
    void fileSaveAs();
    void editFind();
    void editFindNext();
    void editFindPrevious();
    void showBookmarksDialog();
    void addBookmark();
    void addBookmarkFolder();
    void zoomIn();
    void zoomNormal();
    void zoomOut();
    void viewMenuBar();
    void viewToolBars();
    void viewStatusbar();
    void viewPageSource();
    void viewFullScreen(bool enable);
    void viewTextEncoding(QAction *action);
    void setToolBarsVisible(bool);
    void toolBarViewActionTriggered(bool checked);

    void webSearch();
    void clearPrivateData();
    void toggleInspector(bool enable);
    void aboutApplication();
    void downloadManager();
    void selectLineEdit();

    void aboutToShowBackMenu();
    void aboutToShowForwardMenu();
    void aboutToShowViewMenu();
    void aboutToShowWindowMenu();
    void aboutToShowTextEncodingMenu();
    void openActionUrl(QAction *action);
    void showMainMenu();
    void showWindow();
    void swapFocus();
    void onFocusChange(QWidget *, QWidget *);

    void printRequested(QWebFrame *frame);
    void geometryChangeRequested(const QRect &geometry);

    void editToolBars();
    void toolBarDialogClosed(int result);

    void setShowBelowTabBar(bool);
    void insertToolBarsBelowTabBar(int index);
    void toolBarDestroyed(QObject *);
    void restoreDefaultToolBars();

private:
    void retranslate();
    void loadDefaultState();
    void setupMenu();
    void setupToolBars();
    void showAndFocus(QWidget *);
    void updateStopReloadActionText(bool loading);
    void updateToolBarsVisible();
    void makeToolBarAction(QAction *action);

private:
    QMenu *m_fileMenu;
    QAction *m_fileNewWindowAction;
    QAction *m_fileOpenFileAction;
    QAction *m_fileOpenLocationAction;
    QAction *m_fileSaveAsAction;
    QAction *m_fileImportBookmarksAction;
    QAction *m_fileExportBookmarksAction;
    QAction *m_filePrintPreviewAction;
    QAction *m_filePrintAction;
    QAction *m_filePrivateBrowsingAction;
    QAction *m_fileCloseWindow;
    QAction *m_fileQuit;

    QMenu *m_editMenu;
    QAction *m_editUndoAction;
    QAction *m_editRedoAction;
    QAction *m_editCutAction;
    QAction *m_editCopyAction;
    QAction *m_editPasteAction;
    QAction *m_editSelectAllAction;
    QAction *m_editFindAction;
    QAction *m_editFindNextAction;
    QAction *m_editFindPreviousAction;
    QAction *m_editPreferencesAction;

    QMenu *m_viewMenu;
    QAction *m_toolBarMenuAction;
    QAction *m_viewEditToolBarsAction;
    QAction *m_viewShowMenuBarAction;
    QAction *m_viewToolBarsAction;
    QAction *m_viewStatusbarAction;
    QAction *m_viewStopAction;
    QAction *m_viewReloadAction;
    QAction *m_viewZoomInAction;
    QAction *m_viewZoomNormalAction;
    QAction *m_viewZoomOutAction;
    QAction *m_viewZoomTextOnlyAction;
    QAction *m_viewSourceAction;
    QAction *m_viewFullScreenAction;
    QAction *m_viewTextEncodingAction;
    QMenu *m_viewTextEncodingMenu;

    HistoryMenu *m_historyMenu;
    QAction *m_historyBackAction;
    QAction *m_historyForwardAction;
    QAction *m_historyHomeAction;
    QAction *m_historyRestoreLastSessionAction;

    BookmarksMenuBarMenu *m_bookmarksMenu;
    QAction *m_bookmarksShowAllAction;
    QAction *m_bookmarksAddAction;
    QAction *m_bookmarksAddFolderAction;

    QMenu *m_windowMenu;

    QMenu *m_toolsMenu;
    QAction *m_toolsWebSearchAction;
    QAction *m_toolsClearPrivateDataAction;
    QAction *m_toolsEnableInspector;

    QMenu *m_helpMenu;
    QAction *m_helpChangeLanguageAction;
    QAction *m_helpAboutQtAction;
    QAction *m_helpAboutApplicationAction;

    // Toolbar
    QMenu *m_historyBackMenu;
    QMenu *m_historyForwardMenu;
    QAction *m_toolbarSearchAction;
    QAction *m_lineEditsAction;
    QAction *m_stopReloadAction;
    QAction *m_mainMenuAction;
    QIcon m_reloadIcon;
    QIcon m_stopIcon;
    ToolbarSearch *m_toolbarSearch;
    BookmarksToolBar *m_bookmarksToolbar;
    bool m_toolBarsVisible;
    bool m_editingToolBars;
    QList<QPointer<QToolBar> > m_toolBarsToHide;
    QAction *m_showBelowTabBarAction;
    QList<QToolBar*> m_toolBarsBelowTabBar;
    QToolBar *m_contextMenuToolBar;
    QHash<QString, QAction*> m_toolBarActions;
    QByteArray m_savedState;

    TabWidget *m_tabWidget;

    AutoSaver *m_autoSaver;

    // for showAndFocus()
    QWidget *m_shownWidget;

    // These store if the user requested the menu/status bars visible. They are
    // used to determine if these bars should be reshown when leaving fullscreen.
    bool m_menuBarVisible;
    bool m_statusBarVisible;

    friend class BrowserApplication;
};

#endif // BROWSERMAINWINDOW_H

