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

#include "Item_p.h"
#include <QtTest/QtTest>
#include <memory.h>


// TODO: namespace


using namespace Layouting;

static int st = Item::separatorThickness();

static QtMessageHandler s_original = nullptr;
static QString s_expectedWarning;

class GuestWidget : public QWidget
                  , public GuestInterface
{
    Q_OBJECT
public:
    void setLayoutItem(Item *) override {}
    QWidget * asWidget() override {
        return this;
    }

    QSize minimumSizeHint() const override
    {
        return m_minSize;
    }

    void setMinSize(QSize sz)
    {
        if (sz != m_minSize) {
            m_minSize = sz;
            Q_EMIT layoutInvalidated();
        }
    }

Q_SIGNALS:
    void layoutInvalidated();
private:
    QSize m_minSize = QSize(200, 200);
};

static void fatalWarningsMessageHandler(QtMsgType t, const QMessageLogContext &context, const QString &msg)
{

    s_original(t, context, msg);
    if (t == QtWarningMsg) {
        if (msg.contains(QLatin1String("checkSanity"))) {
            // These will already fail in QVERIFY(checkSanity())
            return;
        }

        if (s_expectedWarning.isEmpty() ||!msg.contains(s_expectedWarning))
            qFatal("Got a warning, category=%s", context.category);
    }
}

static bool serializeDeserializeTest(const std::unique_ptr<ItemContainer> &root)
{
    // Serializes and deserializes a layout
    if (!root->checkSanity())
        return false;

    const QVariantMap serialized = root->toVariantMap();
    ItemContainer root2(root->hostWidget());
    root2.fillFromVariantMap(serialized);

    return root2.checkSanity();
}

static std::unique_ptr<ItemContainer> createRoot()
{
    auto root = new ItemContainer(new QWidget()); // todo WIDGET
    root->setSize({ 1000, 1000 });
    return std::unique_ptr<ItemContainer>(root);
}

static Item* createItem()
{
    static int count = 0;
    count++;
    auto item = new Item(new QWidget());
    item->setGeometry(QRect(0, 0, 200, 200));
    item->setObjectName(QStringLiteral("%1").arg(count));
    auto guest = new GuestWidget();
    guest->setObjectName(item->objectName());
    item->setFrame(guest);
    return item;
}

static ItemContainer* createRootWithSingleItem()
{
    auto root = new ItemContainer(new QWidget()); // todo WIDGET
    root->setSize({ 1000, 1000 });

    Item *item1 = createItem();
    root->insertItem(item1, Location_OnTop);

    return root;
}

class TestMultiSplitter : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void initTestCase()
    {
        s_original = qInstallMessageHandler(fatalWarningsMessageHandler);
    }

private Q_SLOTS:
    void tst_createRoot();
    void tst_insertOne();
    void tst_insertThreeSideBySide();
    void tst_insertTwoHorizontal();
    void tst_insertTwoVertical();
    void tst_insertOnWidgetItem1();
    void tst_insertOnWidgetItem2();
    void tst_insertOnWidgetItem1DifferentOrientation();
    void tst_insertOnWidgetItem2DifferentOrientation();
    void tst_insertOnRootDifferentOrientation();
    void tst_removeItem1();
    void tst_removeItem2();
    void tst_minSize();
    void tst_resize();
    void tst_resizeWithConstraints();
    void tst_availableSize();
    void tst_missingSize();
    void tst_ensureEnoughSize();
    void tst_turnIntoPlaceholder();
    void tst_suggestedRect();
    void tst_insertAnotherRoot();
    void tst_misc1();
    void tst_misc2();
    void tst_misc3();
    void tst_containerGetsHidden();
    void tst_minSizeChanges();
    void tst_numSeparators();
    void tst_separatorMinMax();
};

void TestMultiSplitter::tst_createRoot()
{
    auto root = createRoot();
    QVERIFY(root->isRoot());
    QVERIFY(root->isContainer());
    QVERIFY(!root->isWidget());
    QVERIFY(root->hasOrientation());
    QCOMPARE(root->size(), QSize(1000, 1000));
    QVERIFY(root->checkSanity());
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_insertOne()
{
    auto root = createRoot();
    auto item = createItem();
    root->insertItem(item, Location_OnTop);
    QCOMPARE(root->numChildren(), 1);
    QVERIFY(item->isWidget());
    QVERIFY(!item->isContainer());
    QCOMPARE(root->size(), QSize(1000, 1000));
    QCOMPARE(item->size(), root->size());
    QCOMPARE(item->pos(), QPoint());
    QCOMPARE(item->pos(), root->pos());
    QVERIFY(root->hasChildren());
    QVERIFY(root->checkSanity());
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_insertThreeSideBySide()
{
    // Result is [1, 2, 3]

    auto root = createRoot();
    auto item1 = createItem();
    auto item2 = createItem();
    auto item3 = createItem();

    root->insertItem(item1, Location_OnLeft);
    root->insertItem(item2, Location_OnRight);
    root->insertItem(item3, Location_OnRight);

    QVERIFY(root->checkSanity());
    QCOMPARE(root->numChildren(), 3);
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_insertTwoHorizontal()
{
    auto root = createRoot();
    auto item1 = createItem();
    auto item2 = createItem();
    root->insertItem(item1, Location_OnLeft);
    item1->insertItem(item2, Location_OnRight);
    QVERIFY(root->checkSanity());
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_insertTwoVertical()
{
    auto root = createRoot();
    auto item1 = createItem();
    auto item2 = createItem();
    root->insertItem(item1, Location_OnTop);
    item1->insertItem(item2, Location_OnBottom);
    QVERIFY(root->checkSanity());
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_insertOnWidgetItem1()
{
    // We insert into a widget item instead of in a container. It will insert in the container still
    // Result is still [1, 2, 3]

    auto root = createRoot();
    auto item1 = createItem();
    auto item2 = createItem();
    auto item3 = createItem();
    root->insertItem(item1, Location_OnLeft);
    root->insertItem(item2, Location_OnRight);
    item2->insertItem(item3, Location_OnRight);

    QVERIFY(item3->x() > item2->x());
    QCOMPARE(item3->y(), item2->y());

    QVERIFY(root->checkSanity());
    QCOMPARE(root->numChildren(), 3);
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_insertOnWidgetItem2()
{
    // Same, but result [1, 3, 2]

    auto root = createRoot();
    auto item1 = createItem();
    auto item2 = createItem();
    auto item3 = createItem();
    root->insertItem(item1, Location_OnLeft);
    root->insertItem(item2, Location_OnRight);
    item2->insertItem(item3, Location_OnLeft);

    QVERIFY(item1->x() < item3->x());
    QVERIFY(item3->x() < item2->x());
    QCOMPARE(item3->y(), item2->y());

    QVERIFY(root->checkSanity());
    QCOMPARE(root->numChildren(), 3);
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_insertOnWidgetItem1DifferentOrientation()
{
    // Result [1, 2, |3  |]
    //               |3.1|

    auto root = createRoot();
    auto item1 = createItem();
    auto item2 = createItem();
    auto item3 = createItem();
    auto item31 = createItem();
    root->insertItem(item1, Location_OnLeft);
    QVERIFY(root->checkSanity());

    root->insertItem(item2, Location_OnRight);
    QVERIFY(root->checkSanity());

    item2->insertItem(item3, Location_OnRight);
    QVERIFY(root->checkSanity());

    item3->insertItem(item31, Location_OnBottom);
    QVERIFY(root->checkSanity());

    auto container3 = item3->parentContainer();
    QVERIFY(container3->isContainer());
    QVERIFY(container3 != root.get());
    QVERIFY(root->isHorizontal());
    QVERIFY(container3->isVertical());

    QCOMPARE(root->numChildren(), 3);
    QCOMPARE(container3->numChildren(), 2);

    QVERIFY(item1->x() < item2->x());
    QVERIFY(item3->parentContainer()->x() > item2->x());
    QCOMPARE(item3->x(), 0);
    QCOMPARE(item3->y(), item2->y());
    QCOMPARE(item1->y(), item2->y());

    QVERIFY(item31->y() >= item3->y());
    QCOMPARE(item31->parentContainer(), container3);
    QCOMPARE(item3->parentContainer(), container3);
    QCOMPARE(container3->parentContainer(), root.get());
    QCOMPARE(QPoint(0, 0), item3->pos());
    QCOMPARE(container3->width(), item3->width());
    QCOMPARE(container3->height(), item3->height() + st + item31->height());

    QVERIFY(root->checkSanity());
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_insertOnWidgetItem2DifferentOrientation()
{
    // Result [1, 2, |3 3.2|]
    //               |3.1  |

    auto root = createRoot();
    auto item1 = createItem();
    auto item2 = createItem();
    auto item3 = createItem();
    auto item5 = createItem();
    auto item4 = createItem();
    root->insertItem(item1, Location_OnLeft);
    root->insertItem(item2, Location_OnRight);
    item2->insertItem(item3, Location_OnRight);
    item3->insertItem(item4, Location_OnBottom);
    auto container3Parent = item3->parentContainer();
    item3->insertItem(item5, Location_OnRight);
    QVERIFY(root->checkSanity());
    auto container3 = item3->parentContainer();

    QCOMPARE(container3->parentContainer(), container3Parent);

    QVERIFY(container3->isContainer());
    QVERIFY(container3 != root.get());
    QVERIFY(root->isHorizontal());
    QVERIFY(container3->isHorizontal());
    QVERIFY(container3Parent->isVertical());

    QCOMPARE(root->numChildren(), 3);
    QCOMPARE(container3->numChildren(), 2);
    QCOMPARE(container3Parent->numChildren(), 2);

    QVERIFY(item1->x() < item2->x());
    QCOMPARE(container3->pos(), QPoint(0, 0l));
    QCOMPARE(item3->pos(), container3->pos());
    QVERIFY(container3Parent->x() > item2->x());
    QCOMPARE(item3->y(), item2->y());
    QCOMPARE(item1->y(), item2->y());

    QVERIFY(item4->y() >= item3->y());
    QCOMPARE(item4->parentContainer(), container3Parent);
    QCOMPARE(item3->parentContainer(), container3);
    QCOMPARE(container3Parent->parentContainer(), root.get());
    QCOMPARE(container3->pos(), item3->pos());
    QCOMPARE(container3->width(), item3->width() + item5->width() + st);
    QCOMPARE(container3->height(), item3->height());
    QCOMPARE(container3Parent->height(), item3->height() + st + item4->height());

    QVERIFY(root->checkSanity());
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_insertOnRootDifferentOrientation()
{
    //        [       4     ]
    // Result [1, 2, |3 3.2|]
    //               |3.1  |

    auto root = createRoot();
    auto item1 = createItem();
    auto item2 = createItem();
    auto item3 = createItem();
    auto item31 = createItem();
    auto item32 = createItem();
    auto item4 = createItem();
    root->insertItem(item1, Location_OnLeft);
    root->insertItem(item2, Location_OnRight);
    item2->insertItem(item3, Location_OnRight);
    item3->insertItem(item31, Location_OnBottom);
    item3->insertItem(item32, Location_OnRight);
    root->insertItem(item4, Location_OnTop);

    QCOMPARE(item4->parentContainer(), root.get());
    QCOMPARE(item4->pos(), root->pos());
    QCOMPARE(item4->width(), root->width());

    QVERIFY(root->checkSanity());
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_removeItem1()
{
    //        [       4     ]
    // Result [1, 2, |3 3.2|]
    //               |3.1  |

    auto root = createRoot();
    auto item1 = createItem();
    auto item2 = createItem();
    auto item3 = createItem();
    auto item31 = createItem();
    auto item32 = createItem();
    auto item4 = createItem();
    root->insertItem(item1, Location_OnLeft);
    root->insertItem(item2, Location_OnRight);
    item2->insertItem(item3, Location_OnRight);
    item3->insertItem(item31, Location_OnBottom);
    item3->insertItem(item32, Location_OnRight);
    root->insertItem(item4, Location_OnTop);
    QVERIFY(root->checkSanity());
    QCOMPARE(root->numChildren(), 2);

    root->removeItem(item4);
    QVERIFY(root->checkSanity());
    QCOMPARE(root->numChildren(), 1);

    auto c1 = item1->parentContainer();
    QCOMPARE(c1->pos(), QPoint(0, 0));
    QCOMPARE(c1->width(), root->width());
    QCOMPARE(c1->height(), item1->height());
    QCOMPARE(c1->height(), root->height());

    const int item3and32Width = item3->width() + item32->width() + st;
    root->removeItem(item32);

    QCOMPARE(item3->width(), item3and32Width);
    QVERIFY(root->checkSanity());

    root->removeItem(item31);
    QVERIFY(root->checkSanity());

    QCOMPARE(item2->height(), item3->height());

    QPointer<Item> c3 = item3->parentContainer();
    root->removeItem(c3);
    QVERIFY(c3.isNull());
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_removeItem2()
{
    auto root = createRoot();
    auto item1 = createItem();
    auto item2 = createItem();
    auto item3 = createItem();
    auto item31 = createItem();
    root->insertItem(item1, Location_OnLeft);
    root->insertItem(item2, Location_OnRight);
    item2->insertItem(item3, Location_OnRight);
    item3->insertItem(item31, Location_OnBottom);
    item31->parentContainer()->removeItem(item31);
    item3->parentContainer()->removeItem(item3);
}

void TestMultiSplitter::tst_minSize()
{
    auto root = createRoot();
    auto item1 = createItem();
    auto item2 = createItem();
    auto item22 = createItem();

    item1->m_sizingInfo.minSize = {101, 150};
    item2->m_sizingInfo.minSize = {200, 300};
    item22->m_sizingInfo.minSize = {100, 100};

    root->insertItem(item1, Location_OnLeft);
    root->insertItem(item2, Location_OnRight);
    item2->insertItem(item22, Location_OnBottom);

    QCOMPARE(item2->minSize(), QSize(200, 300));
    QCOMPARE(item2->parentContainer()->minSize(), QSize(200, 300+100+st));

    QCOMPARE(root->minSize(), QSize(101+200+st, 300 + 100 + st));
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_resize()
{
    auto root = createRoot();
    auto item1 = createItem();
    auto item2 = createItem();
    auto item3 = createItem();
    auto item31 = createItem();

    root->insertItem(item1, Location_OnLeft);
    root->insertItem(item2, Location_OnRight);
    root->insertItem(item3, Location_OnRight);

    const int item1Percentage = item1->width() / root->width();
    const int item2Percentage = item1->width() / root->width();
    const int item3Percentage = item1->width() / root->width();

    // Now resize:
    root->resize({2000, 505});
    QVERIFY(root->checkSanity());

    QVERIFY(item1Percentage - (1.0* item1->width() / root->width()) < 0.01);
    QVERIFY(item2Percentage - (1.0* item2->width() / root->width()) < 0.01);
    QVERIFY(item3Percentage - (1.0* item3->width() / root->width()) < 0.01);
    QCOMPARE(root->width(), 2000);
    QCOMPARE(root->height(), 505);
    QCOMPARE(item1->height(), 505);
    QCOMPARE(item2->height(), 505);
    QCOMPARE(item3->height(), 505);

    item3->insertItem(item31, Location_OnBottom);

    QVERIFY(root->checkSanity());
    root->resize({2500, 505});
    QVERIFY(root->checkSanity());
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_resizeWithConstraints()
{
    s_expectedWarning = QStringLiteral("New size doesn't respect size constraints");

    {
        // Test that resizing below minSize isn't permitted.

        auto root = createRoot();
        auto item1 = createItem();
        item1->setMinSize(QSize(500, 500));
        root->insertItem(item1, Location_OnLeft);
        QVERIFY(root->checkSanity());

        root->resize(item1->minSize()); // Still fits
        root->resize(item1->minSize() - QSize(1, 0)); // wouldn't fit
        QCOMPARE(root->size(), item1->size()); // still has the old size
        QVERIFY(serializeDeserializeTest(root));
    }

    {
        // |1|2|3|

        auto root = createRoot();
        auto item1 = createItem();
        auto item2 = createItem();
        auto item3 = createItem();
        root->resize(QSize(2000, 500));
        item1->setMinSize(QSize(500, 500));
        item2->setMinSize(QSize(500, 500));
        item3->setMinSize(QSize(500, 500));
        root->insertItem(item1, Location_OnLeft);
        root->insertItem(item2, Location_OnRight);
        root->insertItem(item3, Location_OnRight);
        QVERIFY(root->checkSanity());

        // TODO: Resize further
    }
    s_expectedWarning.clear();
}

void TestMultiSplitter::tst_availableSize()
{
    auto root = createRoot();
    QCOMPARE(root->availableSize(), QSize(1000, 1000));
    QCOMPARE(root->minSize(), QSize(0, 0));

    auto item1 = createItem();
    auto item2 = createItem();
    auto item3 = createItem();
    item1->m_sizingInfo.minSize = {100, 100};
    item2->m_sizingInfo.minSize = {100, 100};
    item3->m_sizingInfo.minSize = {100, 100};

    root->insertItem(item1, Location_OnLeft);
    QCOMPARE(root->availableSize(), QSize(900, 900));
    QCOMPARE(root->minSize(), QSize(100, 100));
    QCOMPARE(root->neighboursLengthFor(item1, Side1, Qt::Horizontal), 0);
    QCOMPARE(root->neighboursLengthFor(item1, Side2, Qt::Horizontal), 0);
    QCOMPARE(root->neighboursMinLengthFor(item1, Side1, Qt::Horizontal), 0);
    QCOMPARE(root->neighboursMinLengthFor(item1, Side2, Qt::Horizontal), 0);
    QCOMPARE(root->neighbourSeparatorWaste(item1, Side1, Qt::Vertical), 0);
    QCOMPARE(root->neighbourSeparatorWaste(item1, Side2, Qt::Vertical), 0);
    QCOMPARE(root->neighbourSeparatorWaste(item1, Side1, Qt::Horizontal), 0);
    QCOMPARE(root->neighbourSeparatorWaste(item1, Side2, Qt::Horizontal), 0);

    QCOMPARE(root->neighboursLengthFor_recursive(item1, Side1, Qt::Vertical), 0);
    QCOMPARE(root->neighboursLengthFor_recursive(item1, Side2, Qt::Vertical), 0);
    QCOMPARE(root->neighboursLengthFor_recursive(item1, Side1, Qt::Horizontal), 0);
    QCOMPARE(root->neighboursLengthFor_recursive(item1, Side2, Qt::Horizontal), 0);

    root->insertItem(item2, Location_OnLeft);
    QCOMPARE(root->availableSize(), QSize(800 - st, 900));
    QCOMPARE(root->minSize(), QSize(200 + st, 100));
    QCOMPARE(root->neighboursLengthFor(item1, Side1, Qt::Horizontal), item2->width());
    QCOMPARE(root->neighboursLengthFor(item1, Side2, Qt::Horizontal), 0);
    QCOMPARE(root->neighboursLengthFor(item2, Side1, Qt::Horizontal), 0);
    QCOMPARE(root->neighboursLengthFor(item2, Side2, Qt::Horizontal), item1->width());
    QCOMPARE(root->neighboursMinLengthFor(item1, Side1, Qt::Horizontal), item2->minSize().width());
    QCOMPARE(root->neighboursMinLengthFor(item1, Side2, Qt::Horizontal), 0);
    QCOMPARE(root->neighboursMinLengthFor(item2, Side1, Qt::Horizontal), 0);
    QCOMPARE(root->neighboursMinLengthFor(item2, Side2, Qt::Horizontal), item1->minSize().width());

    QCOMPARE(root->neighboursLengthFor_recursive(item1, Side1, Qt::Vertical), 0);
    QCOMPARE(root->neighboursLengthFor_recursive(item1, Side2, Qt::Vertical), 0);
    QCOMPARE(root->neighboursLengthFor_recursive(item1, Side1, Qt::Horizontal), item2->width());
    QCOMPARE(root->neighboursLengthFor_recursive(item1, Side2, Qt::Horizontal), 0);

    root->insertItem(item3, Location_OnBottom);
    QCOMPARE(root->availableSize(), QSize(800 - st, 800 - st));
    QCOMPARE(root->minSize(), QSize(200 + st, 100 + 100 + st));
    QCOMPARE(item3->parentContainer()->neighboursMinLengthFor(item3, Side1, Qt::Vertical), item1->minSize().height());

    auto container2 = item2->parentContainer();
    QCOMPARE(container2->neighboursLengthFor_recursive(item1, Side1, Qt::Vertical), 0);
    QCOMPARE(container2->neighboursLengthFor_recursive(item1, Side2, Qt::Vertical), item3->height());
    QCOMPARE(container2->neighboursLengthFor_recursive(item1, Side1, Qt::Horizontal), item2->width());
    QCOMPARE(container2->neighboursLengthFor_recursive(item1, Side2, Qt::Horizontal), 0);

    // More nesting
    auto item4 = createItem();
    auto item5 = createItem();
    item3->insertItem(item4, Location_OnRight);
    item4->insertItem(item5, Location_OnBottom);

    auto container4 = item4->parentContainer();
    QCOMPARE(container4->neighboursLengthFor_recursive(item4, Side1, Qt::Vertical), item1->height());
    QCOMPARE(container4->neighboursLengthFor_recursive(item4, Side2, Qt::Vertical), item5->height());
    QCOMPARE(container4->neighboursLengthFor_recursive(item4, Side1, Qt::Horizontal), item3->width());
    QCOMPARE(container4->neighboursLengthFor_recursive(item4, Side2, Qt::Horizontal), 0);
    QCOMPARE(container4->neighboursLengthFor_recursive(item5, Side1, Qt::Vertical), item4->height() + item1->height());
    QCOMPARE(container4->neighboursLengthFor_recursive(item5, Side2, Qt::Vertical), 0);
    QCOMPARE(container4->neighboursLengthFor_recursive(item5, Side1, Qt::Horizontal), item3->width());
    QCOMPARE(container4->neighboursLengthFor_recursive(item5, Side2, Qt::Horizontal), 0);

    QCOMPARE(container4->neighbourSeparatorWaste(item4, Side1, Qt::Vertical), 0);
    QCOMPARE(container4->neighbourSeparatorWaste(item4, Side2, Qt::Vertical), st);
    QCOMPARE(container4->neighbourSeparatorWaste(item4, Side1, Qt::Horizontal), 0);
    QCOMPARE(container4->neighbourSeparatorWaste(item4, Side2, Qt::Horizontal), 0);
    QCOMPARE(container4->neighbourSeparatorWaste(item5, Side1, Qt::Vertical), st);
    QCOMPARE(container4->neighbourSeparatorWaste(item5, Side2, Qt::Vertical), 0);
    QCOMPARE(container4->neighbourSeparatorWaste(item5, Side1, Qt::Horizontal), 0);
    QCOMPARE(container4->neighbourSeparatorWaste(item5, Side2, Qt::Horizontal), 0);
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_missingSize()
{
    auto root = createRoot();
    QCOMPARE(root->size(), QSize(1000, 1000));
    QCOMPARE(root->availableSize(), QSize(1000, 1000));

    Item *item1 = createItem();
    item1->setMinSize({100, 100});

    Item *item2 = createItem();
    item2->setMinSize(root->size());

    Item *item3 = createItem();
    item3->setMinSize(root->size() + QSize(100, 200));

    // Test empty root
    QCOMPARE(root->missingSizeFor(item1, Qt::Vertical), QSize(0, 0));
    QCOMPARE(root->missingSizeFor(item2, Qt::Vertical), QSize(0, 0));
    QCOMPARE(root->missingSizeFor(item3, Qt::Vertical), QSize(100, 200));

    // Test with an existing item
    root->insertItem(item1, Location_OnTop);
    QCOMPARE(root->missingSizeFor(item2, Qt::Vertical), item1->minSize() + QSize(0, st));
    QCOMPARE(root->missingSizeFor(item3, Qt::Vertical), item1->minSize() + QSize(0, st) + QSize(100, 200));
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_ensureEnoughSize()
{
    // Tests that the layout's size grows when the item being inserted wouldn't have enough space

    auto root = createRoot(); /// 1000x1000
    Item *item1 = createItem();
    item1->setMinSize({2000, 500});

    // Insert to empty layout:

    root->insertItem(item1, Location_OnLeft);
    QCOMPARE(root->size(), QSize(2000, 1000));
    QCOMPARE(item1->size(), QSize(2000, 1000));
    QCOMPARE(item1->minSize(), root->minSize());
    QVERIFY(root->checkSanity());

    // Insert to non-empty layout
    Item *item2 = createItem();
    item2->setMinSize({2000, 2000});
    root->insertItem(item2, Location_OnRight);
    QVERIFY(root->checkSanity());
    QCOMPARE(root->size(), QSize(item1->minSize().width() + item2->minSize().width() + st, item2->minSize().height()));
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_turnIntoPlaceholder()
{
    auto root = createRoot();
    Item *item1 = createItem();
    Item *item2 = createItem();
    Item *item3 = createItem();
    root->insertItem(item1, Location_OnLeft);
    QVERIFY(item1->isVisible());
    item1->turnIntoPlaceholder();
    QVERIFY(!item1->isVisible());
    QCOMPARE(root->visibleCount_recursive(), 0);
    QCOMPARE(root->count_recursive(), 1);
    QVERIFY(root->checkSanity());

    root->insertItem(item2, Location_OnLeft);
    QVERIFY(root->checkSanity());

    root->insertItem(item3, Location_OnLeft);
    QVERIFY(root->checkSanity());
    QCOMPARE(item2->width() + item3->width() + st, root->width());
    item2->turnIntoPlaceholder();
    QVERIFY(root->checkSanity());
    QCOMPARE(item3->width(), root->width());
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_suggestedRect()
{
    auto root = createRoot();
    root->setSize(QSize(2000, 1000));
    const QSize minSize(100, 100);
    QRect leftRect = root->suggestedDropRect(minSize, nullptr, Location_OnLeft);
    QRect topRect = root->suggestedDropRect(minSize, nullptr, Location_OnTop);
    QRect bottomRect = root->suggestedDropRect(minSize, nullptr, Location_OnBottom);
    QRect rightRect = root->suggestedDropRect(minSize, nullptr, Location_OnRight);

    // Test relative to root:
    QVERIFY(leftRect.width() >= minSize.width());
    QVERIFY(topRect.height() >= minSize.height());
    QVERIFY(bottomRect.height() >= minSize.height());
    QVERIFY(rightRect.width() >= minSize.width());
    QCOMPARE(leftRect.topLeft(), QPoint(0, 0));
    QCOMPARE(leftRect.bottomLeft(), root->rect().bottomLeft());
    QCOMPARE(rightRect.topRight(), root->rect().topRight());
    QCOMPARE(rightRect.bottomRight(), root->rect().bottomRight());
    QCOMPARE(topRect.topLeft(), root->rect().topLeft());
    QCOMPARE(topRect.topRight(), root->rect().topRight());
    QCOMPARE(bottomRect.bottomLeft(), root->rect().bottomLeft());
    QCOMPARE(bottomRect.bottomRight(), root->rect().bottomRight());

    // Test relative to an item
    Item *item1 = createItem();
    item1->setMinSize(QSize(100, 100));
    root->insertItem(item1, Location_OnLeft);
    leftRect = root->suggestedDropRect(minSize, item1, Location_OnLeft);
    topRect = root->suggestedDropRect(minSize, item1, Location_OnTop);
    bottomRect = root->suggestedDropRect(minSize, item1, Location_OnBottom);
    rightRect = root->suggestedDropRect(minSize, item1, Location_OnRight);
    QVERIFY(leftRect.width() >= minSize.width());
    QVERIFY(topRect.height() >= minSize.height());
    QVERIFY(bottomRect.height() >= minSize.height());
    QVERIFY(rightRect.width() >= minSize.width());
    QCOMPARE(leftRect.topLeft(), QPoint(0, 0));
    QCOMPARE(leftRect.bottomLeft(), root->rect().bottomLeft());
    QCOMPARE(rightRect.topRight(), root->rect().topRight());
    QCOMPARE(rightRect.bottomRight(), root->rect().bottomRight());
    QCOMPARE(topRect.topLeft(), root->rect().topLeft());
    QCOMPARE(topRect.topRight(), root->rect().topRight());
    QCOMPARE(bottomRect.bottomLeft(), root->rect().bottomLeft());
    QCOMPARE(bottomRect.bottomRight(), root->rect().bottomRight());


    // Insert another item:
    Item *item2 = createItem();
    item1->setMinSize(QSize(100, 100));
    root->insertItem(item2, Location_OnRight);
    leftRect = root->suggestedDropRect(minSize, item2, Location_OnLeft);
    topRect = root->suggestedDropRect(minSize, item2, Location_OnTop);
    bottomRect = root->suggestedDropRect(minSize, item2, Location_OnBottom);
    rightRect = root->suggestedDropRect(minSize, item2, Location_OnRight);
    QCOMPARE(leftRect.y(), item2->geometry().y());
    QVERIFY(leftRect.x() < item2->geometry().x());
    QVERIFY(leftRect.x() > item1->geometry().x());
    QCOMPARE(rightRect.topRight(), root->rect().topRight());
    QCOMPARE(rightRect.bottomRight(), root->rect().bottomRight());
    QCOMPARE(topRect.topLeft(), item2->geometry().topLeft());
    QCOMPARE(topRect.topRight(), item2->geometry().topRight());
    QCOMPARE(bottomRect.bottomLeft(), item2->geometry().bottomLeft());
    QCOMPARE(bottomRect.bottomRight(), item2->geometry().bottomRight());
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_insertAnotherRoot()
{
    {
        auto root1 = createRoot();
        Item *item1 = createItem();
        root1->insertItem(item1, Location_OnRight);
        QWidget *host1 = root1->hostWidget();

        auto root2 = createRoot();
        Item *item2 = createItem();
        root2->insertItem(item2, Location_OnRight);

        root1->insertItem(root2.get(), Location_OnBottom);

        QCOMPARE(root1->hostWidget(), host1);
        QCOMPARE(root2->hostWidget(), host1);
        for (Item *item : root1->items_recursive()) {
            QCOMPARE(item->hostWidget(), host1);
            QVERIFY(item->isVisible());
        }
        QVERIFY(root1->checkSanity());
        QVERIFY(serializeDeserializeTest(root1));
    }

    {
        auto root1 = createRoot();
        Item *item1 = createItem();
        Item *item2 = createItem();
        root1->insertItem(item1, Location_OnLeft);
        root1->insertItem(item2, Location_OnRight);
        QWidget *host1 = root1->hostWidget();

        auto root2 = createRoot();
        Item *item12 = createItem();
        root2->insertItem(item12, Location_OnRight);

        root1->insertItem(root2.get(), Location_OnTop);

        QCOMPARE(root1->hostWidget(), host1);
        QCOMPARE(root2->hostWidget(), host1);
        for (Item *item : root1->items_recursive()) {
            QCOMPARE(item->hostWidget(), host1);
            QVERIFY(item->isVisible());
        }
        QVERIFY(root1->checkSanity());
        QVERIFY(serializeDeserializeTest(root1));
    }
}

void TestMultiSplitter::tst_misc1()
{
    // Random test1

    auto root = createRoot();
    Item *item1 = createItem();
    Item *item2 = createItem();
    Item *item3 = createItem();
    Item *item4 = createItem();
    Item *item5 = createItem();

    root->insertItem(item1, Location_OnTop);
    item1->insertItem(item2, Location_OnRight);
    root->insertItem(item3, Location_OnBottom);
    item3->insertItem(item4, Location_OnRight);
    root->insertItem(item5, Location_OnLeft);

    QVERIFY(root->checkSanity());
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_misc2()
{
    // Random test1
    // |5|1|2|
    // | |3|4|

    auto root = createRoot();
    Item *item1 = createRootWithSingleItem();
    Item *item2 = createRootWithSingleItem();
    Item *item3 = createRootWithSingleItem();
    Item *item4 = createRootWithSingleItem();
    Item *item5 = createRootWithSingleItem();

    root->insertItem(item1, Location_OnTop);
    QVERIFY(root->checkSanity());
    item1->insertItem(item2, Location_OnRight);
    QVERIFY(root->checkSanity());
    root->insertItem(item3, Location_OnBottom);
    QVERIFY(root->checkSanity());
    item3->insertItem(item4, Location_OnRight);
    QVERIFY(root->checkSanity());

    root->insertItem(item5, Location_OnLeft);
    QVERIFY(root->checkSanity());

    item5->parentContainer()->removeItem(item5);
    QVERIFY(root->checkSanity());
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_misc3()
{
    // Random test1
    // |1|2|3|
    // | |3|4|

    auto root = createRoot();
    Item *item1 = createItem();
    Item *item2 = createItem();
    Item *root2 = createRootWithSingleItem();

    root->insertItem(item1, Location_OnLeft);
    root->insertItem(item2, Location_OnRight);
    root->insertItem(root2, Location_OnRight);
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_containerGetsHidden()
{
    auto root = createRoot();
    Item *item1 = createItem();
    Item *item2 = createItem();
    Item *item3 = createItem();
    root->insertItem(item1, Location_OnLeft);
    QVERIFY(root->checkSanity());

    root->insertItem(item2, Location_OnRight);
    QVERIFY(root->checkSanity());

    item2->insertItem(item3, Location_OnBottom);
    QVERIFY(root->checkSanity());

    item2->turnIntoPlaceholder();
    QVERIFY(root->checkSanity());

    item3->turnIntoPlaceholder();
    QVERIFY(root->checkSanity());
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_minSizeChanges()
{
    auto root = createRoot();
    Item *item1 = createItem();
    root->insertItem(item1, Location_OnLeft);

    root->resize(QSize(200, 200));
    QVERIFY(root->checkSanity());

    auto w1 = static_cast<GuestWidget*>(item1->frame()); // TODO: Static cast not required ?
    w1->setMinSize(QSize(300, 300));
    QVERIFY(root->checkSanity());
    QCOMPARE(root->size(), QSize(300, 300));

    Item *item2 = createItem();
    root->insertItem(item2, Location_OnTop);
    QVERIFY(root->checkSanity());

    root->resize(QSize(1000, 1000));
    QVERIFY(root->checkSanity());

    w1->setMinSize(QSize(700, 700));
    QVERIFY(root->checkSanity());
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_numSeparators()
{
    auto root = createRoot();
    Item *item1 = createItem();
    Item *item2 = createItem();
    Item *item3 = createItem();
    Item *item4 = createItem();

    QCOMPARE(root->separators_recursive().size(), 0);

    root->insertItem(item1, Location_OnLeft);
    QCOMPARE(root->separators_recursive().size(), 0);

    root->insertItem(item2, Location_OnLeft);
    QCOMPARE(root->separators_recursive().size(), 1);

    root->insertItem(item3, Location_OnTop);
    QCOMPARE(root->separators_recursive().size(), 2);
    item3->insertItem(item4, Location_OnRight);
    QCOMPARE(root->separators_recursive().size(), 3);

    root->removeItem(item3);
    QCOMPARE(root->separators_recursive().size(), 2);

    root->clear();
    QCOMPARE(root->separators_recursive().size(), 0);

    Item *item5 = createItem();
    Item *item6 = createItem();

    root->insertItem(item5, Location_OnLeft);
    QCOMPARE(root->separators_recursive().size(), 0);
    root->insertItem(item6, Location_OnLeft, AddingOption_StartHidden);
    QCOMPARE(root->separators_recursive().size(), 0);
    QVERIFY(serializeDeserializeTest(root));
}

void TestMultiSplitter::tst_separatorMinMax()
{
    auto root = createRoot();
    Item *item1 = createItem();
    Item *item2 = createItem();
    root->insertItem(item1, Location_OnLeft);
    root->insertItem(item2, Location_OnLeft);
    item1->setMinSize(QSize(200, 200));
    item2->setMinSize(QSize(200, 200));

    auto separator = root->separators_recursive().at(0);
    QCOMPARE(root->minPosForSeparator(separator), 200);
    QCOMPARE(root->minPosForSeparator_global(separator), 200); // same, since there's no nesting

    QCOMPARE(root->maxPosForSeparator(separator), root->width() - Item::separatorThickness() - 200);
    QCOMPARE(root->maxPosForSeparator(separator), root->width() - Item::separatorThickness() - 200);
    QVERIFY(serializeDeserializeTest(root));
}

QTEST_MAIN(TestMultiSplitter)

#include "tst_multisplitter.moc"
