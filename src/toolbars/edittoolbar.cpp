/**
 * Copyright 2009 Christopher Eby <kreed@kreed.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Arora nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "edittoolbar.h"

#include "animatedspacer.h"

#include <qapplication.h>
#include <qevent.h>
#include <qlayout.h>
#include <qtoolbutton.h>
#include <qwidgetaction.h>

#ifdef Q_OS_MAC
#include <qapplication.h>
#endif

#define DRAG_MIMETYPE QLatin1String("application/x-arora-tool")

EditToolBar::EditToolBar(const QString &name, QWidget *parent)
    : QToolBar(name, parent)
    , m_currentSpacer(0)
    , m_currentSpacerLocation(0)
    , m_editable(false)
    , m_resizing(0)
{
}

EditToolBar::~EditToolBar()
{
    if (m_editable)
        foreach (QWidget *widget, findChildren<QWidget*>())
            widget->setAttribute(Qt::WA_TransparentForMouseEvents, false);
}

void EditToolBar::setEditable(bool editable)
{
    if (editable == m_editable)
        return;

    m_editable = editable;

    setAcceptDrops(editable);

    foreach (QWidget *widget, findChildren<QWidget*>())
        widget->setAttribute(Qt::WA_TransparentForMouseEvents, editable);
}

void EditToolBar::actionEvent(QActionEvent *event)
{
    QToolBar::actionEvent(event);
    if (event->type() == QEvent::ActionAdded) {
        if (m_editable)
            if (QWidget *widget = widgetForAction(event->action()))
                widget->setAttribute(Qt::WA_TransparentForMouseEvents);
        if (event->action()->objectName() == QLatin1String("mainMenu") ||
            event->action()->objectName() == QLatin1String("toolsToolMenu") ||
            event->action()->objectName() == QLatin1String("bookmarksToolMenu"))
            if (QToolButton *button = qobject_cast<QToolButton*>(widgetForAction(event->action())))
                button->setPopupMode(QToolButton::InstantPopup);
    }
}

void EditToolBar::removeCurrentSpacer()
{
    if (m_currentSpacer) {
        if (!m_spacerTimer.isActive())
            m_spacerTimer.start(SPACER_ANIM_PERIOD, this);
        m_currentSpacer->remove();
        m_currentSpacer = 0;
        m_currentSpacerLocation = 0;
    }
}

void EditToolBar::dragEnterEvent(QDragEnterEvent *event)
{
    if (m_editable && event->source() && event->mimeData()->hasFormat(DRAG_MIMETYPE)) {
        event->setDropAction(Qt::MoveAction);
        event->accept();
    } else {
        event->ignore();
    }
}

QAction *EditToolBar::nearestActionAt(const QPoint &pos) const
{
    QAction *nearest = 0;

    QList<QAction*> tools = actions();
    if (tools.isEmpty())
        return 0;

    QWidget *lastWidget = 0;
    for (int i = tools.size(); --i >= 0;) {
        if (QWidget *widget = widgetForAction(tools.at(i))) {
            if (widget->inherits("AnimatedSpacer"))
                continue;
            lastWidget = widget;
            break;
        }
    }
    if (!lastWidget)
        return 0;

    int p;
    int nearestDistance;
    int state;

    if (orientation() == Qt::Vertical) {
        p = pos.y();
        // distance to the end of the tools, where we are placed if we return null
        nearestDistance = qAbs(lastWidget->geometry().bottom() - p);
        state = 0;
    } else if (layoutDirection() == Qt::LeftToRight) {
        p = pos.x();
        nearestDistance = qAbs(lastWidget->geometry().right() - p);
        state = 1;
    } else {
        p = pos.x();
        nearestDistance = qAbs(lastWidget->x() - p);
        state = 2;
    }

    foreach (QAction *action, tools) {
        if (QWidget *widget = widgetForAction(action)) {
            if (widget->inherits("AnimatedSpacer"))
                continue;

            int distance;
            switch (state) {
            case 0:
                distance = qAbs(p - widget->y());
                break;
            case 1:
                distance = qAbs(p - widget->x());
                break;
            case 2:
                distance = qAbs(p - widget->geometry().right());
                break;
            }

            if (distance < nearestDistance) {
                nearest = action;
                nearestDistance = distance;
            }
        }
    }

    return nearest;
}

void EditToolBar::dragMoveEvent(QDragMoveEvent *event)
{
    if (m_editable && event->source() && event->mimeData()->hasFormat(DRAG_MIMETYPE)) {
        event->setDropAction(Qt::MoveAction);
        event->accept();
        QAction *location = nearestActionAt(event->pos());
        if (!m_currentSpacer || location != m_currentSpacerLocation) {
            QByteArray data = event->mimeData()->data(DRAG_MIMETYPE);
            QDataStream in(&data, QIODevice::ReadOnly);
            QSize endSize;
            in >> endSize;

            if (!endSize.isValid()) {
                event->ignore();
                return;
            }

            layout()->setEnabled(false);

            AnimatedSpacer *currentSpacer = m_currentSpacer;
            removeCurrentSpacer();

            QSize startSize(0, 0);
            if (currentSpacer) {
                // offset the layout spacing to avoid a jump on insert
                QSize offset(layout()->spacing(), 0);
                if (orientation() == Qt::Horizontal) {
                    // ensure the toolbar doesn't shrink and grow vertically
                    startSize.setHeight(currentSpacer->height());
                } else {
                    startSize.setWidth(currentSpacer->width());
                    offset.transpose();
                }
                currentSpacer->setOffset(offset);
            }

            m_currentSpacer = new AnimatedSpacer(startSize);
            // it would be nice to get this to calculate the right size even
            // if the widget is expanding
            m_currentSpacer->resize(endSize);
            m_currentSpacerLocation = location;
            insertWidget(location, m_currentSpacer);

            layout()->setEnabled(true);

            if (!m_spacerTimer.isActive())
                m_spacerTimer.start(SPACER_ANIM_PERIOD, this);
        }
    } else {
        event->ignore();
    }
}

void EditToolBar::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
#ifdef Q_OS_MAC
    // Mac sends this event when the cursor moves into a child widget even if
    // WA_TransparentForMouseEvents is set. Ensure we have left the widget to
    // work around this.
    if (geometry().contains(parentWidget()->mapFromGlobal(QCursor::pos())))
        return;
#endif
    if (m_editable) {
        removeCurrentSpacer();
        event->accept();
    }
}

void EditToolBar::dropEvent(QDropEvent *event)
{
    if (m_editable && event->source() && event->mimeData()->hasFormat(DRAG_MIMETYPE)) {
        QByteArray data = event->mimeData()->data(DRAG_MIMETYPE);
        QDataStream in(&data, QIODevice::ReadOnly);
        QSize size;
        quintptr ptr;
        in >> size;
        in >> ptr;

        QAction *action;
        if (ptr && (action = reinterpret_cast<QAction*>(ptr))) {
            // We have to steal the widget from another toolbar if it is already
            // on a toolbar and was dragged from the toolbar dialog.
            if (QWidgetAction *widgetAction = qobject_cast<QWidgetAction*>(action))
                if (QWidget *widget = widgetAction->defaultWidget())
                    if (QToolBar *toolBar = qobject_cast<QToolBar*>(widget->parent()))
                        toolBar->removeAction(action);

            insertAction(m_currentSpacerLocation, action);

            delete m_currentSpacer;
            m_currentSpacer = 0;
            m_currentSpacerLocation = 0;
        } else {
            removeCurrentSpacer();
        }

        event->setDropAction(Qt::MoveAction);
        event->accept();
    } else {
        event->ignore();
    }
}

void EditToolBar::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_spacerTimer.timerId()) {
        layout()->setEnabled(false);
        bool active = false;
        foreach (QObject *child, children())
            if (AnimatedSpacer *spacer = qobject_cast<AnimatedSpacer*>(child))
                if (spacer->step())
                   active = true;
        layout()->setEnabled(true);
        if (!active)
            m_spacerTimer.stop();
    } else {
        QToolBar::timerEvent(event);
    }
}

void EditToolBar::mousePressEvent(QMouseEvent *event)
{
    if (m_editable && event->button() == Qt::LeftButton) {
        QList<QAction*> tools = actions();
        QWidget *widget = 0;
        QAction *action = 0;
        int i = tools.size();
        while (--i >= 0) {
            action = tools.at(i);
            QWidget *child = widgetForAction(action);
            if (child && !child->isHidden() && child->geometry().contains(event->pos())) {
                widget = child;
                break;
            }
        }

        if (widget) {
            if (event->modifiers() & Qt::ShiftModifier) {
                if (widget->sizePolicy().expandingDirections() & orientation()) {
                    m_resizing = widget;
                    QPoint centerPoint = m_resizing->geometry().center();
                    int center;
                    if (orientation() == Qt::Horizontal) {
                        m_resizeFrom = event->x();
                        m_resizeMin = m_resizing->sizeHint().width();
                        m_resizeMax = width() - layout()->sizeHint().width() + m_resizeMin;
                        center = centerPoint.x();
                    } else {
                        m_resizeFrom = event->y();
                        m_resizeMin = m_resizing->sizeHint().height();
                        m_resizeMax = height() - layout()->sizeHint().height() + m_resizeMin;
                        center = centerPoint.y();
                    }
                    m_resizeDirection = m_resizeFrom < center ? -1 : 1;
                }
            } else {
                QAction *after = i == tools.size() - 1 ? 0 : tools.at(i + 1);

                m_currentSpacer = new AnimatedSpacer(widgetForAction(action)->sizeHint());
                m_currentSpacer->resize(QSize(0, 0));
                m_currentSpacerLocation = after;
                insertWidget(after, m_currentSpacer);

                removeAction(action);

                QEvent event(QEvent::LayoutRequest);
                qApp->notify(this, &event);

                QPixmap pixmap(widget->sizeHint());
                pixmap.fill(Qt::transparent);
                widget->render(&pixmap, QPoint(), QRegion(), QWidget::DrawChildren);

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
                if (drag->exec(Qt::MoveAction) != Qt::MoveAction)
                    insertAction(after, action);
#ifdef Q_OS_MAC
                qApp->restoreOverrideCursor();
#endif
            }
            return;
        }
    }

    QToolBar::mousePressEvent(event);
}

void EditToolBar::mouseMoveEvent(QMouseEvent *event)
{
    if (m_resizing) {
        bool horizontal = orientation() == Qt::Horizontal;

        int point = horizontal ? event->x() : event->y();
        int distance = (point - m_resizeFrom) * m_resizeDirection;

        QSize size_ = m_resizing->size();
        int &size = horizontal ? size_.rwidth() : size_.rheight();

        size += distance;
        if (size < m_resizeMin) {
            size = m_resizeMin;
        } else if (size > m_resizeMax) {
            if (isFloating())
                resize(width() + distance, height());
            else
                size = m_resizeMax;
        }

        m_resizing->setMinimumSize(size_);
        m_resizing->setMaximumSize(size_);
        m_resizeFrom = point;
    } else {
        QToolBar::mouseMoveEvent(event);
    }
}

void EditToolBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_resizing) {
        Qt::Orientation orientation = EditToolBar::orientation();
        int totalSize = 0;
        foreach (QObject *child, children())
            if (QWidget *widget = qobject_cast<QWidget*>(child))
                if (widget->sizePolicy().expandingDirections() & orientation)
                    totalSize += orientation == Qt::Horizontal ? widget->width() : widget->height();

        foreach (QObject *child, children()) {
            if (QWidget *widget = qobject_cast<QWidget*>(child)) {
                QSizePolicy policy = widget->sizePolicy();
                if (policy.expandingDirections() & orientation) {
                    if (orientation == Qt::Horizontal)
                        policy.setHorizontalStretch(widget->width() * UCHAR_MAX / totalSize);
                    else
                        policy.setVerticalStretch(widget->height() * UCHAR_MAX / totalSize);
                    widget->setSizePolicy(policy);
                }
            }
        }

        m_resizing->setMinimumSize(QSize(0, 0));
        m_resizing->setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
        m_resizing = 0;
    }

    QToolBar::mouseReleaseEvent(event);
}
