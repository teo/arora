/*
 * Copyright 2009 Christopher Eby <kreed@kreed.org>
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

#ifndef TOOLBARDIALOG_H
#define TOOLBARDIALOG_H

#include <qdialog.h>
#include "ui_toolbardialog.h"

class BrowserMainWindow;

class ToolBarDialog : public QDialog, Ui_ToolBarDialog
{
    Q_OBJECT

signals:
    void newToolBarRequested(const QString &name);
    void toolButtonStyleChanged(Qt::ToolButtonStyle);
    void iconSizeChanged(const QSize &);
    void restoreDefaultToolBars();

public:
    ToolBarDialog(BrowserMainWindow *parent);

    void addActions(const QList<QAction*> &actions);

private slots:
    void addToolBar();
    void changeToolButtonStyle(int);
    void changeIconSize(int);
    void dialogButtonClicked(QAbstractButton *);
};

#endif
