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
#include "multisplitter/MultiSplitterLayout_p.h"
#include "multisplitter/Anchor_p.h"
#include "Logging_p.h"
#include "Item_p.h"

using namespace KDDockWidgets;

Separator::Separator(KDDockWidgets::Anchor *anchor, QWidgetAdapter *parent)
    : QWidgetAdapter(parent)
    , m_anchor(anchor)
{
    Q_ASSERT(anchor);
    setVisible(true);

    const int thickness = Item::separatorThickness();
    if (isVertical())
        setFixedWidth(thickness);
    else
        setFixedHeight(thickness);
}

bool Separator::isVertical() const
{
    return m_anchor->isVertical();
}

int Separator::position() const
{
    return isVertical() ? x() : y();
}

void Separator::onMousePress()
{
    m_anchor->onMousePress();
}

void Separator::onMouseMove(QPoint globalPos)
{
    m_anchor->onMouseMoved(parentWidget()->mapFromGlobal(globalPos));
}

void Separator::onMouseRelease()
{
    m_anchor->onMouseReleased();
}

void Separator::move(int p)
{
    if (isVertical()) {
        QWidgetAdapter::move(p, y());
    } else {
        QWidgetAdapter::move(x(), p);
    }
}
