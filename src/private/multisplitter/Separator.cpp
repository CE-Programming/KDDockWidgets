/*
  This file is part of KDDockWidgets.

  Copyright (C) 2018-2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Sérgio Martins <sergio.martins@kdab.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Separator_p.h"
#include "Logging_p.h"
#include "Item_p.h"

#include <QMouseEvent>
#include <QRubberBand>
#include <QApplication>

using namespace Layouting;

Separator* Separator::s_separatorBeingDragged = nullptr;

static SeparatorFactoryFunc s_separatorFactoryFunc = nullptr;

Separator::Separator(QWidget *hostWidget)
    : QWidget(hostWidget)
{
}

Separator::~Separator()
{
    if (isBeingDragged())
        s_separatorBeingDragged = nullptr;
}

bool Separator::isVertical() const
{
    return m_orientation == Qt::Vertical;
}

void Separator::move(int p)
{
    if (isVertical()) {
        QWidget::move(x(), p);
    } else {
        QWidget::move(p, y());
    }
}

Qt::Orientation Separator::orientation() const
{
    return m_orientation;
}

void Separator::mousePressEvent(QMouseEvent *)
{
    s_separatorBeingDragged = this;

    qCDebug(separators) << "Drag started";

    if (lazyResizeEnabled()) {
        setLazyPosition(position());
        m_lazyResizeRubberBand->show();
    }
}

void Separator::mouseMoveEvent(QMouseEvent *ev)
{
    if (!isBeingDragged())
        return;

    if (!(qApp->mouseButtons() & Qt::LeftButton)) {
        qCDebug(separators) << Q_FUNC_INFO << "Ignoring spurious mouse event. Someone ate our ReleaseEvent";
        onMouseReleased();
        return;
    }

#ifdef Q_OS_WIN
    // Try harder, Qt can be wrong, if mixed with MFC
    const bool mouseButtonIsReallyDown = (GetKeyState(VK_LBUTTON) & 0x8000) || (GetKeyState(VK_RBUTTON) & 0x8000);
    if (!mouseButtonIsReallyDown) {
        qCDebug(mouseevents) << Q_FUNC_INFO << "Ignoring spurious mouse event. Someone ate our ReleaseEvent";
        onMouseReleased();
        return;
    }
#endif

    const int positionToGoTo = Layouting::pos(mapToParent(ev->pos()), m_orientation);
    const int minPos = m_parentContainer->minPosForSeparator_global(this);
    const int maxPos = m_parentContainer->maxPosForSeparator_global(this);

    if (positionToGoTo < minPos || positionToGoTo > maxPos)
        return;

    m_lastMoveDirection = positionToGoTo < position() ? Side1
                                                      : (positionToGoTo > position() ? Side2
                                                                                     : Side2); // Last case shouldn't happen though.

    if (m_lazyResize)
        setLazyPosition(positionToGoTo);
    else
        m_parentContainer->requestSeparatorMove(this, positionToGoTo - position());
}

void Separator::mouseReleaseEvent(QMouseEvent *)
{
    onMouseReleased();
}

void Separator::onMouseReleased()
{
    if (m_lazyResizeRubberBand) {
        m_lazyResizeRubberBand->hide();
        m_parentContainer->requestSeparatorMove(this, m_lazyPosition - position());
    }

    s_separatorBeingDragged = nullptr;
}

bool Separator::lazyResizeEnabled() const
{
    return m_options & SeparatorOption::LazyResize;
}

void Separator::setGeometry(QRect r)
{
    if (r != m_geometry) {
        m_geometry = r;
        QWidget::setGeometry(r);
        setVisible(true);
    }
}

int Separator::position() const
{
    const QPoint topLeft = m_geometry.topLeft();
    return isVertical() ? topLeft.y() : topLeft.x();
}

QWidget *Separator::hostWidget() const
{
    return parentWidget();
}

void Separator::init(ItemContainer *parentContainer, Qt::Orientation orientation, SeparatorOptions options)
{
    m_parentContainer = parentContainer;
    m_orientation = orientation;
    m_options = options;
    m_lazyResizeRubberBand = (options & SeparatorOption::LazyResize) ? new QRubberBand(QRubberBand::Line, hostWidget())
                                                                     : nullptr;
    setVisible(true);
}

ItemContainer *Separator::parentContainer() const
{
    return m_parentContainer;
}

void Separator::setGeometry(int pos, int pos2, int length)
{
    QRect newGeo = m_geometry;
    if (isVertical()) {
        // The separator itself is horizontal
        newGeo.setSize(QSize(length, Item::separatorThickness));
        newGeo.moveTo(pos2, pos);
    } else {
        // The separator itself is vertical
        newGeo.setSize(QSize(Item::separatorThickness, length));
        newGeo.moveTo(pos, pos2);
    }

    setGeometry(newGeo);
}

bool Separator::isResizing()
{
    return s_separatorBeingDragged != nullptr;
}

void Separator::setSeparatorFactoryFunc(SeparatorFactoryFunc func)
{
    s_separatorFactoryFunc = func;
}

Separator* Separator::createSeparator(QWidget *host)
{
    if (s_separatorFactoryFunc)
        return s_separatorFactoryFunc(host);

    return new Separator(host);
}

void Separator::setLazyPosition(int pos)
{
    if (m_lazyPosition != pos) {
        m_lazyPosition = pos;

        QRect geo = geometry();
        if (isVertical()) {
            geo.moveLeft(pos);
        } else {
            geo.moveTop(pos);
        }

        m_lazyResizeRubberBand->setGeometry(geo);
    }
}

bool Separator::isBeingDragged() const
{
    return s_separatorBeingDragged == this;
}
