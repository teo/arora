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

#include "toolbardialog.h"

#include "browsermainwindow.h"

#include <qinputdialog.h>

ToolBarDialog::ToolBarDialog(BrowserMainWindow *parent)
    : QDialog(parent)
{
    setupUi(this);

    m_toolArea->setIconSize(parent->iconSize());

    m_toolButtonModes->addItem(tr("Icons"));
    m_toolButtonModes->addItem(tr("Text"));
    m_toolButtonModes->addItem(tr("Text Beside Icons"));
    m_toolButtonModes->addItem(tr("Text Under Icons"));
#if QT_VERSION >= 0x040600
    m_toolButtonModes->addItem(tr("System Default"));
#endif

    m_toolButtonModes->setCurrentIndex(parent->toolButtonStyle());
    m_iconSizes->setValue(parent->iconSize().width());

    connect(m_toolButtonModes, SIGNAL(currentIndexChanged(int)),
            this, SLOT(changeToolButtonStyle(int)));
    connect(m_iconSizes, SIGNAL(valueChanged(int)),
            this, SLOT(changeIconSize(int)));
    connect(m_newToolBarButton, SIGNAL(clicked()),
            this, SLOT(addToolBar()));
    connect(m_buttonBox, SIGNAL(clicked(QAbstractButton*)),
            this, SLOT(dialogButtonClicked(QAbstractButton*)));
    connect(this, SIGNAL(iconSizeChanged(QSize)),
            m_toolArea, SLOT(setIconSize(QSize)));
    connect(this, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)),
            m_toolArea, SLOT(setToolButtonStyle(Qt::ToolButtonStyle)));
}

void ToolBarDialog::changeToolButtonStyle(int i)
{
    emit toolButtonStyleChanged(Qt::ToolButtonStyle(i));
}

void ToolBarDialog::changeIconSize(int size)
{
    emit iconSizeChanged(QSize(size, size));
}

void ToolBarDialog::addActions(const QList<QAction*> &actions)
{
    m_toolArea->addActions(actions);
}

void ToolBarDialog::addToolBar()
{
    bool ok;
    QString text = QInputDialog::getText(this, tr("Add ToolBar"),
            tr("Enter a name for the new toolbar:"), QLineEdit::Normal,
            QString(), &ok);
    if (ok && !text.isEmpty())
        emit newToolBarRequested(text);
}

void ToolBarDialog::dialogButtonClicked(QAbstractButton *button)
{
    switch (m_buttonBox->buttonRole(button)) {
    case QDialogButtonBox::ResetRole: {
        emit restoreDefaultToolBars();
        BrowserMainWindow *parent = qobject_cast<BrowserMainWindow*>(ToolBarDialog::parent());
        m_toolButtonModes->setCurrentIndex(parent->toolButtonStyle());
        m_iconSizes->setValue(parent->iconSize().width());
        break;
    }
    case QDialogButtonBox::AcceptRole:
        accept();
        break;
    case QDialogButtonBox::RejectRole:
        reject();
        break;
    default:
        break;
    }
}
