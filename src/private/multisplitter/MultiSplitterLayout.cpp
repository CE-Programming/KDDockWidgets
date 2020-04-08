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

#include "MultiSplitterLayout_p.h"
#include "Logging_p.h"
#include "MultiSplitter_p.h"
#include "Frame_p.h"
#include "FloatingWindow_p.h"
#include "DockWidgetBase.h"
#include "LastPosition_p.h"
#include "DockRegistry_p.h"
#include "Config.h"
#include "Separator_p.h"
#include "FrameworkWidgetFactory.h"
#include "LayoutSaver.h"

#include <QAction>
#include <QEvent>
#include <QtMath>
#include <QScopedValueRollback>

#define INDICATOR_MINIMUM_LENGTH 100

using namespace KDDockWidgets;
using namespace Layouting;

MultiSplitterLayout::MultiSplitterLayout(MultiSplitter *parent)
    : QObject(parent)
    , m_multiSplitter(parent)
    , m_rootItem(new ItemContainer(parent))
{
    Q_ASSERT(parent);

    DockRegistry::self()->registerLayout(this);

    setSize(parent->size());

    qCDebug(multisplittercreation()) << "MultiSplitter";

    connect(m_rootItem, &ItemContainer::numItemsChanged, this, &MultiSplitterLayout::widgetCountChanged);
    connect(m_rootItem, &ItemContainer::numVisibleItemsChanged, this, &MultiSplitterLayout::visibleWidgetCountChanged);

    clear();

    // Initialize min size
    updateSizeConstraints();
    m_inCtor = false;
}

MultiSplitterLayout::~MultiSplitterLayout()
{
    qCDebug(multisplittercreation) << "~MultiSplitter" << this;
    m_inDestructor = true;
    const auto anchors = m_anchors;
    qDeleteAll(anchors);
    if (m_rootItem->hostWidget() == multiSplitter())
        delete m_rootItem;
    DockRegistry::self()->unregisterLayout(this);
}

MultiSplitter *MultiSplitterLayout::multiSplitter() const
{
    return m_multiSplitter;
}

bool MultiSplitterLayout::validateInputs(QWidgetOrQuick *widget,
                                         Location location,
                                         const Frame *relativeToFrame, AddingOption option) const
{
    if (!widget) {
        qWarning() << Q_FUNC_INFO << "Widget is null";
        return false;
    }

    const bool isDockWidget = qobject_cast<DockWidgetBase*>(widget);
    const bool isStartHidden = option & AddingOption_StartHidden;

    if (!qobject_cast<Frame*>(widget) && !qobject_cast<MultiSplitter*>(widget) && !isDockWidget) {
        qWarning() << "Unknown widget type" << widget;
        return false;
    }

    if (isDockWidget != isStartHidden) {
        qWarning() << "Wrong parameters" << isDockWidget << isStartHidden;
        return false;
    }

    if (relativeToFrame && relativeToFrame == widget) {
        qWarning() << "widget can't be relative to itself";
        return false;
    }

    Item *item = itemForFrame(qobject_cast<Frame*>(widget));

    if (contains(item)) {
        qWarning() << "MultiSplitterLayout::addWidget: Already contains" << widget;
        return false;
    }// TODO: check for widget changing parent

    if (location == Location_None) {
        qWarning() << "MultiSplitterLayout::addWidget: not adding to location None";
        return false;
    }

    const bool relativeToThis = relativeToFrame == nullptr;

    Item *relativeToItem = itemForFrame(relativeToFrame);
    if (!relativeToThis && !contains(relativeToItem)) {
        qWarning() << "MultiSplitterLayout::addWidget: Doesn't contain relativeTo:"
                   << "; relativeToFrame=" << relativeToFrame
                   << "; relativeToItem=" << relativeToItem
                   << "; options=" << option;
        return false;
    }

    return true;
}

void MultiSplitterLayout::addWidget(QWidgetOrQuick *w, Location location, Frame *relativeToWidget, AddingOption option)
{
    auto frame = qobject_cast<Frame*>(w);
    qCDebug(addwidget) << Q_FUNC_INFO << w
                       << "; location=" << locationStr(location)
                       << "; relativeTo=" << relativeToWidget
                       << "; size=" << size()
                       << "; w.size=" << w->size()
                       << "; w.min=" << Layouting::widgetMinSize(w)
                       << "; frame=" << frame
                       << "; option=" << option;

    if (itemForFrame(frame) != nullptr) {
        // Item already exists, remove it.
        // Changing the frame parent will make the item clean itself up. It turns into a placeholder and is removed by unrefOldPlaceholders
        frame->setParent(nullptr); // so ~Item doesn't delete it
        frame->setLayoutItem(nullptr); // so Item is destroyed, as there's no refs to it
    }

    // Make some sanity checks:
    if (!validateInputs(w, location, relativeToWidget, option))
        return;

    Item *relativeTo = itemForFrame(relativeToWidget);
    if (!relativeTo)
        relativeTo = m_rootItem;

    Item *newItem = nullptr;

    Frame::List frames = framesFrom(w);

    if (frame) {
        newItem = new Item(multiSplitter());
        newItem->setFrame(frame);
        qDebug() << "geo frame" << frame->geometry();
    } else if (auto dw = qobject_cast<DockWidgetBase*>(w)) {
        newItem = new Item(multiSplitter());
        newItem->setFrame(new Frame(dw));
        qDebug() << "geo dw" << dw->geometry();
    } else if (auto ms = qobject_cast<MultiSplitter*>(w)) {
        newItem = ms->multiSplitterLayout()->rootItem();
        Q_ASSERT(newItem->hostWidget() != multiSplitter());
        newItem->setHostWidget(multiSplitter());

        qDebug() << ms->multiSplitterLayout()->frames()
                 << ms->multiSplitterLayout()->frames().at(0)->parentWidget()
                 << multiSplitter()
                 << ms;

        delete ms;
    }

    Q_ASSERT(!newItem->geometry().isEmpty());
    relativeTo->insertItem(newItem, Layouting::Location(location));

    unrefOldPlaceholders(frames);
}

QString MultiSplitterLayout::affinityName() const
{
    if (auto ms = multiSplitter()) {
        if (auto mainWindow = ms->mainWindow()) {
            return mainWindow->affinityName();
        } else if (auto fw = ms->floatingWindow()) {
            return fw->affinityName();
        }
    }

    return QString();
}

void MultiSplitterLayout::addMultiSplitter(MultiSplitter *sourceMultiSplitter,
                                           Location location,
                                           Frame *relativeTo)
{
    qCDebug(addwidget) << Q_FUNC_INFO << sourceMultiSplitter << location << relativeTo;
    addWidget(sourceMultiSplitter, location, relativeTo);
}

void MultiSplitterLayout::removeItem(Item *item)
{
    if (!item || m_inDestructor)
        return;

    item->parentContainer()->removeItem(item);


    Q_EMIT widgetRemoved(item);     // TODO Remove.
}

bool MultiSplitterLayout::contains(const Item *item) const
{
    return m_rootItem->contains_recursive(item);
}

bool MultiSplitterLayout::contains(const Frame *frame) const
{
    return itemForFrame(frame) != nullptr;
}

Item *MultiSplitterLayout::itemAt(QPoint p) const
{
    return m_rootItem->itemAt_recursive(p);
}

void MultiSplitterLayout::clear()
{
    m_rootItem->clear();
}

int MultiSplitterLayout::visibleCount() const
{
    return m_rootItem->visibleCount_recursive();
}

int MultiSplitterLayout::placeholderCount() const
{
    return count() - visibleCount();
}

void MultiSplitterLayout::setAnchorBeingDragged(Anchor *anchor)
{
    m_anchorBeingDragged = anchor;
}

int MultiSplitterLayout::numVisibleAnchors() const
{
    int count = 0;
    for (Anchor *a : m_anchors) {
        if (a->separatorWidget()->isVisible())
            count++;
    }

    return count;
}

void MultiSplitterLayout::updateSizeConstraints()
{
    const QSize newMinSize = m_rootItem->minSize();
    qCDebug(sizing) << Q_FUNC_INFO << "Updating size constraints from" << m_minSize
                    << "to" << newMinSize;

    setMinimumSize(newMinSize);
}

void MultiSplitterLayout::emitVisibleWidgetCountChanged()
{
    if (!m_inDestructor)
        Q_EMIT visibleWidgetCountChanged(visibleCount());
}

Item *MultiSplitterLayout::itemForFrame(const Frame *frame) const
{
    if (!frame)
        return nullptr;

    return m_rootItem->itemForFrame(frame);
}

Frame::List MultiSplitterLayout::framesFrom(QWidgetOrQuick *frameOrMultiSplitter) const
{
    if (auto frame = qobject_cast<Frame*>(frameOrMultiSplitter))
        return { frame };

    if (auto msw = qobject_cast<MultiSplitter*>(frameOrMultiSplitter))
        return msw->multiSplitterLayout()->frames();

    return {};
}

Frame::List MultiSplitterLayout::frames() const
{
    const Item::List items = m_rootItem->items_recursive();

    Frame::List result;
    result.reserve(items.size());

    for (Item *item : items) {
        if (auto f = static_cast<Frame*>(item->frame()))
            result.push_back(f);
    }

    return result;
}

QVector<DockWidgetBase *> MultiSplitterLayout::dockWidgets() const
{
    DockWidgetBase::List result;
    const Frame::List frames = this->frames();

    for (Frame *frame : frames)
        result << frame->dockWidgets();

    return result;
}

void MultiSplitterLayout::restorePlaceholder(DockWidgetBase *dw, Item *item, int tabIndex)
{
    if (item->isPlaceholder()) {
        Frame *newFrame = Config::self().frameworkWidgetFactory()->createFrame(multiSplitter());
        item->restorePlaceholder(newFrame);
    }

    Frame *frame = qobject_cast<Frame*>(item->frame());
    Q_ASSERT(frame);

    if (tabIndex != -1 && frame->dockWidgetCount() >= tabIndex) {
        frame->insertWidget(dw, tabIndex);
    } else {
        frame->addWidget(dw);
    }

    frame->setVisible(true);
}

bool MultiSplitterLayout::checkSanity() const
{
    return m_rootItem->checkSanity();
}

void MultiSplitterLayout::unrefOldPlaceholders(const Frame::List &framesBeingAdded) const
{
    for (Frame *frame : framesBeingAdded) {
        for (DockWidgetBase *dw : frame->dockWidgets()) {
            if (Item *existingItem = dw->lastPosition()->layoutItem()) {
                if (contains(existingItem)) { // We're only interested in placeholders from this layout
                    dw->lastPosition()->removePlaceholders(this);
                }
            }
        }
    }
}

void MultiSplitterLayout::dumpDebug() const
{
    m_rootItem->dumpLayout();
}

void MultiSplitterLayout::setSize(QSize size)
{
    if (size != this->size()) {
        m_rootItem->resize(size);
        m_resizing = true;
        Q_EMIT sizeChanged(size);
        m_resizing = false; // TODO: m_resizing needed ?
    }
}

void MultiSplitterLayout::setContentLength(int value, Qt::Orientation o)
{
    if (o == Qt::Vertical) {
        // Setting the width
        setSize({value, size().height()});
    } else {
        // Setting the height
        setSize({size().width(), value});
    }
}

int MultiSplitterLayout::length(Qt::Orientation o) const
{
    return o == Qt::Vertical ? width()
                             : height();
}

void MultiSplitterLayout::setMinimumSize(QSize sz)
{
    if (sz != m_rootItem->minSize()) {
        m_rootItem->setMinSize(sz);
        setSize(size().expandedTo(m_rootItem->minSize())); // Increase size incase we need to
        Q_EMIT minimumSizeChanged(sz);
    }
    qCDebug(sizing) << Q_FUNC_INFO << "minSize = " << m_minSize;
}

const ItemList MultiSplitterLayout::items() const
{
    return m_rootItem->items_recursive();
}

Item *MultiSplitterLayout::rootItem() const
{
    return m_rootItem;
}

QRect MultiSplitterLayout::rectForDrop(const QWidgetOrQuick *widget, Location location, const Item *relativeTo) const
{
    // TODO
    Q_UNUSED(widget);
    Q_UNUSED(location);
    Q_UNUSED(relativeTo);

    const QSize min = Layouting::widgetMinSize(widget);


    if (relativeTo) {
        ItemContainer *container = relativeTo->parentContainer();
        QRect rect = container->suggestedDropRect(min, relativeTo, Layouting::Location(location));
        return container->mapToRoot(rect);
    } else {
        return m_rootItem->suggestedDropRect(min, nullptr, Layouting::Location(location));
    }
}

bool MultiSplitterLayout::deserialize(const LayoutSaver::MultiSplitterLayout &)
{
    return true;
}

LayoutSaver::MultiSplitterLayout MultiSplitterLayout::serialize() const
{
    LayoutSaver::MultiSplitterLayout l;
    return l;
}
