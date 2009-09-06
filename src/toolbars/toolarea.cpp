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

#include "toolarea.h"

#include "flowlayout.h"

#include <qevent.h>
#include <qlabel.h>
#include <qtoolbutton.h>
#include <qwidgetaction.h>

#ifdef Q_OS_MAC
#include <qapplication.h>
#endif

#define DRAG_MIMETYPE QLatin1String("application/x-arora-tool")

ToolArea::ToolArea(QWidget *parent)
    : QWidget(parent)
{
    setAcceptDrops(true);
    setLayout(new FlowLayout(20, 20, 20));
}

void ToolArea::setIconSize(const QSize &size)
{
    if (size == m_iconSize)
        return;
    m_iconSize = size;
    foreach (QToolButton *button, findChildren<QToolButton*>())
        button->setIconSize(size);
}

void ToolArea::setToolButtonStyle(Qt::ToolButtonStyle style)
{
    foreach (QToolButton *button, findChildren<QToolButton*>())
        button->setToolButtonStyle(style);
}

static QPixmap render(QWidget *widget, const QSize &maxSize)
{
    QSize size = widget->sizeHint();
    QPixmap pixmap(qMin(size.width(), maxSize.width()), qMin(size.height(), maxSize.height()));
    pixmap.fill(Qt::transparent);
    widget->render(&pixmap, QPoint(), QRegion(), QWidget::DrawChildren);
    return pixmap;
}

void ToolArea::addActions(const QList<QAction*> &actions)
{
    foreach (QAction *action, actions) {
        QWidget *widget;

        if (QWidgetAction *widgetAction = qobject_cast<QWidgetAction*>(action)) {
            widget = new QWidget;
            QLayout *boxLayout = new QVBoxLayout;

            QLabel *icon = new QLabel;
            icon->setPixmap(::render(widgetAction->defaultWidget(), size() / 2));
            icon->setAlignment(Qt::AlignCenter);
            icon->setProperty("ACTION", quintptr(action));
            boxLayout->addWidget(icon);

            QLabel *text = new QLabel;
            text->setText(action->text());
            text->setAlignment(Qt::AlignCenter);
            boxLayout->addWidget(text);

            widget->setLayout(boxLayout);
        } else {
            QToolButton *toolButton = new QToolButton;
            toolButton->setIconSize(m_iconSize);
            toolButton->setDefaultAction(action);
            // for some reason mac ignores this
            toolButton->setAutoRaise(true);
            // don't show the icon text
            toolButton->setText(action->text());
            toolButton->setEnabled(true);
            toolButton->setAttribute(Qt::WA_TransparentForMouseEvents);
            widget = toolButton;
        }

        layout()->addWidget(widget);
    }
}

void ToolArea::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QWidget *widget = 0;
        foreach (QObject *object, children()) {
            if (QWidget *child = qobject_cast<QWidget*>(object)) {
                if (child->geometry().contains(event->pos())) {
                    widget = child;
                    break;
                }
            }
        }
        if (!widget)
            return;

        QPixmap pixmap;
        QAction *action;
        if (QToolButton *button = qobject_cast<QToolButton*>(widget)) {
            action = button->defaultAction();
            pixmap = ::render(button, size() / 2);
        } else if (QLabel *label = qobject_cast<QLabel*>(widget->childAt(widget->mapFromParent(event->pos())))) {
            action = reinterpret_cast<QAction*>(label->property("ACTION").value<quintptr>());
            pixmap = *label->pixmap();
        } else {
            return;
        }

        QMimeData *mimeData = new QMimeData;
        QByteArray data;
        QDataStream out(&data, QIODevice::WriteOnly);
        out << QSize(pixmap.width(), pixmap.height());
        out << quintptr(action);
        mimeData->setData(DRAG_MIMETYPE, data);

        QDrag *drag = new QDrag(this);
        drag->setMimeData(mimeData);
        drag->setPixmap(pixmap);
        drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));

#ifdef Q_OS_MAC
        // Mac crashes when dropping without this. (??)
        qApp->setOverrideCursor(cursor());
#endif
        drag->exec(Qt::MoveAction);
#ifdef Q_OS_MAC
        qApp->restoreOverrideCursor();
#endif
    } else {
        QWidget::mousePressEvent(event);
    }
}

void ToolArea::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(DRAG_MIMETYPE))
        event->acceptProposedAction();
    QWidget::dragEnterEvent(event);
}

void ToolArea::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat(DRAG_MIMETYPE))
        event->acceptProposedAction();
    QWidget::dropEvent(event);
}
