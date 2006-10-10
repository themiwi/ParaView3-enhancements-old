/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqFlatTreeView.cxx,v $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

/// \file pqFlatTreeView.cxx
/// \date 3/27/2006

#include "pqFlatTreeView.h"

#include <QApplication>
#include <QEvent>
#include <QHeaderView>
#include <QList>
#include <QPainter>
#include <QPaintEvent>
#include <QPersistentModelIndex>
#include <QPoint>
#include <QRect>
#include <QScrollBar>
#include <QStyle>


class pqFlatTreeViewColumn
{
public:
  pqFlatTreeViewColumn();
  ~pqFlatTreeViewColumn() {}

public:
  int Width;
  bool Selected;
};


class pqFlatTreeViewItem
{
public:
  pqFlatTreeViewItem();
  ~pqFlatTreeViewItem();

public:
  pqFlatTreeViewItem *Parent;
  QList<pqFlatTreeViewItem *> Items;
  QPersistentModelIndex Index;
  QList<pqFlatTreeViewColumn *> Cells;
  int ContentsY;
  int Indent;
  bool Expandable;
  bool Expanded;
  bool RowSelected;
};


class pqFlatTreeViewItemRows : public QList<int> {};

class pqFlatTreeViewInternal : public QList<pqFlatTreeViewColumn *> {};


pqFlatTreeViewColumn::pqFlatTreeViewColumn()
{
  this->Width = 0;
  this->Selected = false;
}


pqFlatTreeViewItem::pqFlatTreeViewItem()
  : Items(), Index(), Cells()
{
  this->Parent = 0;
  this->ContentsY = 0;
  this->Indent = 0;
  this->Expandable = false;
  this->Expanded = false;
  this->RowSelected = false;
}

pqFlatTreeViewItem::~pqFlatTreeViewItem()
{
  // Clean up the child items.
  QList<pqFlatTreeViewItem *>::Iterator iter = this->Items.begin();
  for( ; iter != this->Items.end(); ++iter)
    {
    delete *iter;
    }

  this->Items.clear();
  this->Parent = 0;

  // Clean up the cells.
  QList<pqFlatTreeViewColumn *>::Iterator jter = this->Cells.begin();
  for( ; jter != this->Cells.end(); ++jter)
    {
    delete *jter;
    }

  this->Cells.clear();
}


int pqFlatTreeView::TextMargin = 4;
int pqFlatTreeView::DoubleTextMargin = 2 * pqFlatTreeView::TextMargin;
int pqFlatTreeView::PipeLength = 4;

pqFlatTreeView::pqFlatTreeView(QWidget *p)
  : QAbstractItemView(p)
{
  this->HeaderView = 0;
  this->Root = new pqFlatTreeViewItem();
  this->Internal = new pqFlatTreeViewInternal();
  this->ItemHeight = 0;
  this->IndentWidth = 0;
  this->ContentsWidth = 0;
  this->ContentsHeight = 0;
  this->FontChanged = false;
  this->ManageSizes = true;
  this->InUpdateWidth = false;

  // Set the default edit triggers.
  this->setEditTriggers(QAbstractItemView::DoubleClicked |
      QAbstractItemView::EditKeyPressed);

  // Set up the default header view.
  QHeaderView *headerView = new QHeaderView(Qt::Horizontal, this->viewport());
  headerView->setClickable(false);
  headerView->setSortIndicatorShown(false);
  headerView->setResizeMode(QHeaderView::Interactive);
  this->setHeader(headerView);
}

pqFlatTreeView::~pqFlatTreeView()
{
  if(this->Root)
    {
    delete this->Root;
    }

  if(this->Internal)
    {
    // Clean up the columns.
    QList<pqFlatTreeViewColumn *>::Iterator iter = this->Internal->begin();
    for( ; iter != this->Internal->end(); ++iter)
      {
      delete *iter;
      }

    delete this->Internal;
    }
}

QModelIndex pqFlatTreeView::indexAt(const QPoint &point) const
{
  if(!this->Root || this->Root->Items.size() == 0 || !this->HeaderView)
    {
    return QModelIndex();
    }

  // Convert the coordinates to contents space.
  int px = point.x() + this->horizontalOffset();
  int py = point.y() + this->verticalOffset();
  if(px > this->ContentsWidth && py > this->ContentsHeight)
    {
    return QModelIndex();
    }

  // Search for the item.
  pqFlatTreeViewItem *item = this->getNextVisibleItem(this->Root);
  while(item)
    {
    if(py >= item->ContentsY && py < item->ContentsY + this->ItemHeight)
      {
      // Check which column the point is in.
      int column = this->HeaderView->logicalIndexAt(point);
      return item->Index.sibling(item->Index.row(), column);
      }

    item = this->getNextVisibleItem(item);
    }

  return QModelIndex();
}

QRect pqFlatTreeView::visualRect(const QModelIndex &index) const
{
  if(!index.isValid() || index.model() != this->model())
    {
    return QRect();
    }

  pqFlatTreeViewItem *item = this->getItem(index);
  if(item && this->HeaderView)
    {
    int column = index.column();
    int vx = this->HeaderView->sectionPosition(column);
    
    // If the index is for column 0, account for the indent.
    int columnWidth = this->HeaderView->sectionSize(column);
    if(column == 0)
      {
      vx += item->Indent;
      columnWidth -= item->Indent;
      }

    int visualWidth = 0;
    if(column < item->Cells.size())
      {
      visualWidth = item->Cells[column]->Width;
      }

    // The column may be sized less than the text width.
    if(columnWidth < visualWidth)
      {
      visualWidth = columnWidth;
      }

    // Make sure the vertical connector is not included in the height.
    int visualHeight = this->ItemHeight - pqFlatTreeView::PipeLength;

    // The rectangle should be returned in viewport coordinates.
    vx -= this->horizontalOffset();
    int vy = item->ContentsY + pqFlatTreeView::PipeLength -
        this->verticalOffset();
    return QRect(vx, vy, visualWidth, visualHeight);
    }

  return QRect();
}

void pqFlatTreeView::scrollTo(const QModelIndex &index,
    QAbstractItemView::ScrollHint hint)
{
  pqFlatTreeViewItem *item = this->getItem(index);
  if(item && this->HeaderView)
    {
    bool atTop = hint == QAbstractItemView::PositionAtTop;
    if(hint == QAbstractItemView::EnsureVisible)
      {
      if(item->ContentsY < this->verticalOffset())
        {
        atTop = true;
        }
      else if(item->ContentsY + this->ItemHeight <= this->verticalOffset() +
          this->viewport()->height())
        {
        return;
        }
      }

    int cy = 0;
    if(atTop)
      {
      if(this->ContentsHeight - item->ContentsY > this->viewport()->height())
        {
        cy = item->ContentsY;
        if(!this->HeaderView->isHidden())
          {
          cy -= this->HeaderView->size().height();
          }

        this->verticalScrollBar()->setValue(cy);
        }
      else
        {
        this->verticalScrollBar()->setValue(
            this->verticalScrollBar()->maximum());
        }
      }
    else
      {
      cy = item->ContentsY + this->ItemHeight - this->viewport()->height();
      if(cy < 0)
        {
        this->verticalScrollBar()->setValue(0);
        }
      else
        {
        this->verticalScrollBar()->setValue(cy);
        }
      }
    }
}

void pqFlatTreeView::keyboardSearch(const QString &)
{
  // TODO
}

void pqFlatTreeView::setModel(QAbstractItemModel *itemModel)
{
  QAbstractItemModel *current = this->model();
  if(current)
    {
    this->disconnect(current,
        SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
        this, SLOT(rowsRemoved(const QModelIndex &, int, int)));
    }

  if(this->HeaderView)
    {
    this->HeaderView->setModel(itemModel);
    }

  QAbstractItemView::setModel(itemModel);
  current = this->model();
  if(current)
    {
    this->connect(current, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
        this, SLOT(rowsRemoved(const QModelIndex &, int, int)));
    }
}

void pqFlatTreeView::setSelectionModel(QItemSelectionModel *itemSelectionModel)
{
  QItemSelectionModel *oldModel = this->selectionModel();
  if(oldModel)
    {
    // Clear the selection flags.

    // Disconnect from the selection model signals.
    disconnect(oldModel,
        SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
        this, SLOT(changeCurrent(const QModelIndex &, const QModelIndex &)));
    disconnect(oldModel,
        SIGNAL(currentRowChanged(const QModelIndex &, const QModelIndex &)),
        this,
        SLOT(changeCurrentRow(const QModelIndex &, const QModelIndex &)));
    disconnect(oldModel,
        SIGNAL(currentColumnChanged(const QModelIndex &, const QModelIndex &)),
        this,
        SLOT(changeCurrentColumn(const QModelIndex &, const QModelIndex &)));
    disconnect(oldModel,
        SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
        this,
        SLOT(changeSelection(const QItemSelection &, const QItemSelection &)));
    }

  if(this->HeaderView)
    {
    this->HeaderView->setSelectionModel(itemSelectionModel);
    }

  QAbstractItemView::setSelectionModel(itemSelectionModel);
  QItemSelectionModel *newModel = this->selectionModel();
  if(newModel)
    {
    // Listen to the selection model signals.
    connect(newModel,
        SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
        this, SLOT(changeCurrent(const QModelIndex &, const QModelIndex &)));
    connect(newModel,
        SIGNAL(currentRowChanged(const QModelIndex &, const QModelIndex &)),
        this,
        SLOT(changeCurrentRow(const QModelIndex &, const QModelIndex &)));
    connect(newModel,
        SIGNAL(currentColumnChanged(const QModelIndex &, const QModelIndex &)),
        this,
        SLOT(changeCurrentColumn(const QModelIndex &, const QModelIndex &)));
    connect(newModel,
        SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
        this,
        SLOT(changeSelection(const QItemSelection &, const QItemSelection &)));
    }
}

void pqFlatTreeView::reset()
{
  // Clean up the view items.
  if(this->Root)
    {
    delete this->Root;
    this->Root = 0;
    }

  // Allocate a new root and finish reseting the view.
  this->Root = new pqFlatTreeViewItem();
  QAbstractItemView::reset();
}

void pqFlatTreeView::setRootIndex(const QModelIndex &index)
{
  // If the root index is changed outside the view, the current view
  // items may need to be cleaned up.
  if(this->Root && this->Root->Items.size() > 0)
    {
    if(this->rootIndex() == index)
      {
      return;
      }

    // If the index is different, the view items should be cleaned up.
    delete this->Root;
    this->Root = new pqFlatTreeViewItem();
    }

  if(this->HeaderView)
    {
    this->HeaderView->setRootIndex(index);
    }

  QAbstractItemView::setRootIndex(index);
  if(this->Root)
    {
    this->Root->Index = this->rootIndex();

    // Traverse the model to set up the view items. Only set up the
    // visible items.
    this->addChildItems(this->Root, 1);
    this->layoutItems();
    this->viewport()->update();
    }
}

bool pqFlatTreeView::eventFilter(QObject *object, QEvent *e)
{
  if(object && object == this->HeaderView)
    {
    if(e->type() == QEvent::Show || e->type() == QEvent::Hide)
      {
      // When the header changes visibility, the layout needs to
      // be updated.
      int point = 0;
      if(e->type() == QEvent::Show)
        {
        point = this->HeaderView->size().height();
        }

      QStyleOptionViewItem options = this->viewOptions();
      pqFlatTreeViewItem *item = this->getNextVisibleItem(this->Root);
      while(item)
        {
        this->layoutItem(item, point, options.fontMetrics);
        item = this->getNextVisibleItem(item);
        }

      // Update the contents size and repaint the viewport.
      this->ContentsHeight = point;
      this->updateContentsWidth();
      this->updateScrollBars();
      this->viewport()->update();
      }
    }

  return QAbstractItemView::eventFilter(object, e);
}

void pqFlatTreeView::setHeader(QHeaderView *headerView)
{
  if(headerView)
    {
    // Clean up the current header.
    if(this->HeaderView)
      {
      delete this->HeaderView;
      }

    this->HeaderView = headerView;
    this->HeaderView->setParent(this->viewport());
    this->HeaderView->setModel(this->model());
    if(this->HeaderView->objectName().isEmpty())
      {
      this->HeaderView->setObjectName("HeaderView");
      }

    // Connect the horizontal scrollbar to the header.
    connect(this->horizontalScrollBar(), SIGNAL(valueChanged(int)),
        this->HeaderView, SLOT(setOffset(int)));

    // Listen to the header signals.
    connect(this->HeaderView, SIGNAL(sectionResized(int,int,int)),
            this, SLOT(handleSectionResized(int,int,int)));
    connect(this->HeaderView, SIGNAL(sectionMoved(int,int,int)),
            this, SLOT(handleSectionMoved(int,int,int)));
    this->HeaderView->setFocusProxy(this);

    // Listen to the header's show/hide event.
    this->HeaderView->installEventFilter(this);

    // If the viewport is visible, resize the header to fit in
    // the viewport.
    if(this->viewport()->isVisible())
      {
      QSize headerSize = this->HeaderView->sizeHint();
      headerSize.setWidth(this->viewport()->width());
      this->HeaderView->resize(headerSize);
      this->HeaderView->show();
      }
    }
}

void pqFlatTreeView::setColumnSizeManaged(bool managed)
{
  if(this->ManageSizes != managed)
    {
    this->ManageSizes = managed;
    if(this->HeaderView && !this->HeaderView->isHidden())
      {
      this->updateContentsWidth();
      this->updateScrollBars();
      this->viewport()->update();
      }
    }
}

void pqFlatTreeView::expand(const QModelIndex &index)
{
  pqFlatTreeViewItem *item = this->getItem(index);
  if(item && item->Expandable && !item->Expanded)
    {
    // An expandable item might not have the child items created
    // yet. If the item ends up having no children, it should be
    // marked as not expandable.
    QRect area;
    bool noChildren = item->Items.size() == 0;
    if(noChildren)
      {
      this->addChildItems(item, item->Parent->Items.size());
      if(item->Items.size() == 0)
        {
        // Repaint the item to remove the expandable button.
        item->Expandable = false;
        area.setRect(0, item->ContentsY, this->ContentsWidth,
            this->ItemHeight);
        area.translate(-this->horizontalOffset(), -this->verticalOffset());
        this->viewport()->update(area);
        return;
        }
      }

    item->Expanded = true;

    // Update the position of the items following the expanded item.
    int point = item->ContentsY + this->ItemHeight;
    QStyleOptionViewItem options = this->viewOptions();
    pqFlatTreeViewItem *next = this->getNextVisibleItem(item);
    while(next)
      {
      this->layoutItem(next, point, options.fontMetrics);
      next = this->getNextVisibleItem(next);
      }

    // Update the contents size.
    this->ContentsHeight = point;
    bool widthChanged = this->updateContentsWidth();
    this->updateScrollBars();

    if(widthChanged)
      {
      this->viewport()->update();
      }
    else
      {
      // Repaint the area from the item to the end of the view.
      area.setRect(0, item->ContentsY, this->ContentsWidth,
          this->ContentsHeight - item->ContentsY);
      area.translate(-this->horizontalOffset(), -this->verticalOffset());
      this->viewport()->update(area);
      }
    }
}

void pqFlatTreeView::collapse(const QModelIndex &index)
{
  pqFlatTreeViewItem *item = this->getItem(index);
  if(item && item->Expandable && item->Expanded)
    {
    item->Expanded = false;

    // Update the positions for the items following this one.
    int point = item->ContentsY + this->ItemHeight;
    QStyleOptionViewItem options = this->viewOptions();
    pqFlatTreeViewItem *next = this->getNextVisibleItem(item);
    while(next)
      {
      this->layoutItem(next, point, options.fontMetrics);
      next = this->getNextVisibleItem(next);
      }

    // Update the contents size.
    int oldHeight = this->ContentsHeight;
    this->ContentsHeight = point;
    this->updateScrollBars();

    // Repaint the area from the item to the end of the view.
    QRect area(0, item->ContentsY, this->ContentsWidth, oldHeight -
        item->ContentsY);
    area.translate(-this->horizontalOffset(), -this->verticalOffset());
    this->viewport()->update(area);
    }
}

void pqFlatTreeView::rowsRemoved(const QModelIndex &parentIndex, int, int)
{
  // Get the view item for the parent index. If the view item
  // doesn't exist, it is not visible and no update is necessary.
  pqFlatTreeViewItem *item = this->getItem(parentIndex);
  if(item)
    {
    // If the root is empty, reset the preferred size list.
    if(this->Root->Items.size() == 0)
      {
      this->resetInternal(this->model()->columnCount(this->Root->Index));
      }

    // Layout the following items now that the model has finished
    // removing the items.
    int point = item->ContentsY + this->ItemHeight;
    QStyleOptionViewItem options = this->viewOptions();
    pqFlatTreeViewItem *next = this->getNextVisibleItem(item);
    while(next)
      {
      this->layoutItem(next, point, options.fontMetrics);
      next = this->getNextVisibleItem(next);
      }

    // Update the contents size.
    int oldHeight = this->ContentsHeight;
    this->ContentsHeight = point;
    bool widthChanged = this->updateContentsWidth();
    this->updateScrollBars();

    if(widthChanged)
      {
      this->viewport()->update();
      }
    else
      {
      // Repaint the area from the item to the end of the view.
      QRect area(0, item->ContentsY, this->ContentsWidth, oldHeight -
          item->ContentsY);
      area.translate(-this->horizontalOffset(), -this->verticalOffset());
      this->viewport()->update(area);
      }
    }
}

void pqFlatTreeView::rowsInserted(const QModelIndex &parentIndex,
    int start, int end)
{
  // Get the view item for the parent index. If the view item
  // doesn't exist, it is not visible and no update is necessary.
  // If the item is expandable but is collapsed, only add the new
  // rows if view items have previously been added to the item.
  // Otherwise, the new rows will get added when the item is
  // expanded along with the previous rows.
  pqFlatTreeViewItem *item = this->getItem(parentIndex);
  if(item && !(item->Expandable && !item->Expanded &&
      item->Items.size() == 0))
    {
    // Create view items for the new rows. Put the new items on
    // a temporary list.
    QModelIndex index;
    QList<pqFlatTreeViewItem *> newItems;
    pqFlatTreeViewItem *child = 0;
    int count = item->Items.size() + end - start + 1;
    for( ; end >= start; end--)
      {
      index = this->model()->index(start, 0, parentIndex);
      if(index.isValid())
        {
        child = new pqFlatTreeViewItem();
        if(child)
          {
          child->Parent = item;
          child->Index = index;
          newItems.prepend(child);
          this->addChildItems(child, count);
          }
        }
      }

    if(newItems.size() > 0)
      {
      // If the item has only one child, adding more can make the
      // first child expandable. If the one child already has child
      // items, it should be set as expanded, since the items were
      // previously visible.
      if(item->Items.size() == 1)
        {
        item->Items[0]->Expandable = item->Items[0]->Items.size() > 0;
        item->Items[0]->Expanded = item->Items[0]->Expandable;
        }
      else if(item->Items.size() == 0 && item->Parent)
        {
        item->Expandable = item->Parent->Items.size() > 1;
        }

      // Add the new items to the correct location in the item.
      QList<pqFlatTreeViewItem *>::Iterator iter = newItems.begin();
      for( ; iter != newItems.end(); ++iter, ++start)
        {
        item->Items.insert(start, *iter);
        }

      // Layout the visible items following the changed item
      // including the newly added items. Only layout the items
      // if they are visible.
      if(this->HeaderView && (!item->Expandable || item->Expanded))
        {
        int point = 0;
        if(item != this->Root)
          {
          point = item->ContentsY + this->ItemHeight;
          }
        else if(!this->HeaderView->isHidden())
          {
          point = this->HeaderView->size().height();
          }

        QStyleOptionViewItem options = this->viewOptions();
        pqFlatTreeViewItem *next = this->getNextVisibleItem(item);
        while(next)
          {
          this->layoutItem(next, point, options.fontMetrics);
          next = this->getNextVisibleItem(next);
          }

        // Update the contents size.
        this->ContentsHeight = point;
        bool widthChanged = this->updateContentsWidth();
        this->updateScrollBars();

        if(widthChanged)
          {
          this->viewport()->update();
          }
        else
          {
          // Repaint the area from the item to the end of the view.
          QRect area(0, item->ContentsY, this->ContentsWidth,
              this->ContentsHeight - item->ContentsY);
          area.translate(-this->horizontalOffset(), -this->verticalOffset());
          this->viewport()->update(area);
          }
        }
      }
    }
}

void pqFlatTreeView::rowsAboutToBeRemoved(const QModelIndex &parentIndex,
    int start, int end)
{
  QAbstractItemView::rowsAboutToBeRemoved(parentIndex, start, end);

  // Get the view item for the parent index. If the view item
  // doesn't exist, it is not visible and no update is necessary.
  pqFlatTreeViewItem *item = this->getItem(parentIndex);
  if(item)
    {
    // Remove the items. Re-layout the views when the model is
    // done removing the items.
    for( ; end >= start; end--)
      {
      if(end < item->Items.size())
        {
        delete item->Items.takeAt(end);
        }
      }

    // If the view item was expandable, make sure that is still true.
    if(item->Expandable)
      {
      item->Expandable = item->Items.size() > 0;
      item->Expanded = item->Expandable && item->Expanded;
      }
    }
}

void pqFlatTreeView::dataChanged(const QModelIndex &topLeft,
    const QModelIndex &bottomRight)
{
  QAbstractItemView::dataChanged(topLeft, bottomRight);

  // The the changed indexes must have the same parent.
  QModelIndex parentIndex = topLeft.parent();
  if(parentIndex != bottomRight.parent())
    {
    return;
    }

  pqFlatTreeViewItem *parentItem = this->getItem(parentIndex);
  if(parentItem && parentItem->Items.size() > 0)
    {
    // If the corresponding view items exist, zero out the
    // affected columns in the items.
    bool itemsVisible = !parentItem->Expandable || parentItem->Expanded;
    QStyleOptionViewItem options = this->viewOptions();
    pqFlatTreeViewItem *item = 0;
    int startPoint = 0;
    int point = 0;
    int start = topLeft.column();
    int end = bottomRight.column();
    for(int i = topLeft.row(); i <= bottomRight.row(); i++)
      {
      if(i < parentItem->Items.size())
        {
        item = parentItem->Items[i];
        if(i == 0)
          {
          startPoint = item->ContentsY;
          }

        for(int j = start; j <= end && j < item->Cells.size(); j++)
          {
          item->Cells[j]->Width = 0;
          }

        // If the items are visible, update the layout.
        if(itemsVisible)
          {
          point = item->ContentsY;
          this->layoutItem(item, point, options.fontMetrics);
          }
        }
      }

    // If the items are visible, repaint the affected area.
    if(itemsVisible)
      {
      bool widthChanged = this->updateContentsWidth();
      this->updateScrollBars();

      if(widthChanged)
        {
        this->viewport()->update();
        }
      else
        {
        QRect area(0, startPoint, this->ContentsWidth, point - startPoint);
        area.translate(-this->horizontalOffset(), -this->verticalOffset());
        this->viewport()->update(area);
        }
      }
    }
}

void pqFlatTreeView::mouseDoubleClickEvent(QMouseEvent *e)
{
  QModelIndex index = this->indexAt(e->pos());
  if(index.isValid() && index.column() == 0)
    {
    pqFlatTreeViewItem *item = this->getItem(index);
    if(item && item->Expandable)
      {
      if(item->Expanded)
        {
        this->collapse(index);
        }
      else
        {
        this->expand(index);
        }

      return;
      }
    }

  QAbstractItemView::mouseDoubleClickEvent(e);
}

void pqFlatTreeView::mousePressEvent(QMouseEvent *e)
{
  if(this->HeaderView)
    {
    int column = this->HeaderView->logicalIndexAt(e->pos());
    int py = e->y() + this->verticalOffset();
    if(column == 0 && py < this->ContentsHeight)
      {
      // Find the item at the mouse location.
      pqFlatTreeViewItem *item = this->getNextVisibleItem(this->Root);
      while(item)
        {
        if(py >= item->ContentsY && py < item->ContentsY + this->ItemHeight)
          {
            if(item->Expandable)
            {
            int px = this->HeaderView->sectionPosition(0) + item->Indent;
            int ex = e->x() + this->horizontalOffset();
            if(ex < px && ex > px - this->IndentWidth &&
                py > item->ContentsY + pqFlatTreeView::PipeLength)
              {
              if(item->Expanded)
                {
                this->collapse(item->Index);
                }
              else
                {
                this->expand(item->Index);
                }

              return;
              }
            }

          break;
          }

        item = this->getNextVisibleItem(item);
        }
      }
    }

  QAbstractItemView::mousePressEvent(e);
}

QModelIndex pqFlatTreeView::moveCursor(
    QAbstractItemView::CursorAction cursorAction,
    Qt::KeyboardModifiers)
{
  QModelIndex current = this->selectionModel()->currentIndex();
  pqFlatTreeViewItem *item = this->getItem(current);
  if(item)
    {
    int i = 0;
    int rows = 0;
    pqFlatTreeViewItem *newItem = 0;
    switch(cursorAction)
      {
      case QAbstractItemView::MoveUp:
      case QAbstractItemView::MovePrevious:
        {
        if(this->selectionBehavior() != QAbstractItemView::SelectColumns)
          {
          newItem = this->getPreviousVisibleItem(item);
          if(newItem)
            {
            current = newItem->Index;
            }
          }

        break;
        }
      case QAbstractItemView::MoveDown:
      case QAbstractItemView::MoveNext:
        {
        if(this->selectionBehavior() != QAbstractItemView::SelectColumns)
          {
          newItem = this->getNextVisibleItem(item);
          if(newItem)
            {
            current = newItem->Index;
            }
          }

        break;
        }
      case QAbstractItemView::MoveLeft:
        {
        if(this->selectionBehavior() == QAbstractItemView::SelectColumns)
          {
          // TODO
          }
        else if(current.column() == 0)
          {
          if(item->Expandable && item->Expanded)
            {
            this->collapse(current);
            }
          else
            {
            // Find the next parent that is expandable.
            newItem = item->Parent;
            while(newItem)
              {
              if(newItem->Expandable)
                {
                break;
                }
              else if(newItem->Items.size() > 1)
                {
                break;
                }

              newItem = newItem->Parent;
              }

            if(newItem && newItem != this->Root)
              {
              current = newItem->Index;
              }
            }
          }

        break;
        }
      case QAbstractItemView::MoveRight:
        {
        if(this->selectionBehavior() == QAbstractItemView::SelectColumns)
          {
          // TODO
          }
        else if(current.column() == 0)
          {
          if(item->Expandable && !item->Expanded)
            {
            this->expand(current);
            }
          else if(item->Expandable || item->Items.size() > 1)
            {
            // Move to the first child.
            current = item->Items[0]->Index;
            }
          }

        break;
        }
      case QAbstractItemView::MovePageUp:
        {
        if(this->ItemHeight)
          {
          rows = (this->viewport()->height() / this->ItemHeight) - 1;
          if(rows < 1)
            {
            rows = 1;
            }

          for(i = 0; i < rows; i++)
            {
            newItem = this->getPreviousVisibleItem(item);
            if(!newItem)
              {
              break;
              }

            item = newItem;
            }

          current = item->Index;
          }

        break;
        }
      case QAbstractItemView::MovePageDown:
        {
        if(this->ItemHeight)
          {
          rows = (this->viewport()->height() / this->ItemHeight) - 1;
          if(rows < 1)
            {
            rows = 1;
            }

          for(i = 0; i < rows; i++)
            {
            newItem = this->getNextVisibleItem(item);
            if(!newItem)
              {
              break;
              }

            item = newItem;
            }

          current = item->Index;
          }

        break;
        }
      case QAbstractItemView::MoveHome:
        {
        newItem = this->getNextVisibleItem(this->Root);
        if(newItem && newItem != item)
          {
          current = newItem->Index;
          }

        break;
        }
      case QAbstractItemView::MoveEnd:
        {
        newItem = this->getLastVisibleItem();
        if(newItem && newItem != item)
          {
          current = newItem->Index;
          }

        break;
        }
      }
    }

  return current;
}

bool pqFlatTreeView::isIndexHidden(const QModelIndex &index) const
{
  if(!this->Root)
    {
    // If the root is not allocated, nothing is shown.
    return true;
    }

  // Get the row hierarchy from the index and its ancestors.
  pqFlatTreeViewItemRows rowList;
  if(!this->getIndexRowList(index, rowList))
    {
    // The index is not in the view hierarchy, so it is hidden. This
    // may be the case if the root of the view is not the root of the
    // model (see setRootIndex).
    return true;
    }

  // Walk the hierarchy from the root to the index to see if
  // any of the parent items are collapsed.
  pqFlatTreeViewItem *item = this->Root;
  QList<int>::Iterator iter = rowList.begin();
  for( ; iter != rowList.end(); ++iter)
    {
    if(*iter >= 0 && *iter < item->Items.size())
      {
      item = item->Items[*iter];
      if(item->Parent->Expandable && !item->Parent->Expanded)
        {
        // The index is hidden by a collapsed ancestor.
        return true;
        }
      }
    else
      {
      // Index is not in the view hierarchy yet, so it is hidden.
      return true;
      }
    }

  // None of the view item ancestors are closed, so it is not hidden.
  return false;
}

void pqFlatTreeView::setSelection(const QRect &viewRect,
    QItemSelectionModel::SelectionFlags command)
{
  if(!this->Root || !this->HeaderView || !this->selectionModel())
    {
    return;
    }

  if(this->selectionMode() == QAbstractItemView::NoSelection)
    {
    return;
    }

  // Transform the rectangle to contents coordinates.
  pqFlatTreeViewItem *item = 0;
  QItemSelection selection;
  QModelIndex index;
  QList<int> columnList;
  QList<int>::Iterator iter;
  QRect localRect = viewRect;
  localRect.translate(this->horizontalOffset(), this->verticalOffset());
  if(this->selectionBehavior() == QAbstractItemView::SelectColumns ||
      this->selectionBehavior() == QAbstractItemView::SelectItems)
    {
    // Find the columns in the rectangle. Use the header to
    // determine the column order.
    int cx = 0;
    int logical;
    for(int i = 0; i < this->HeaderView->count(); i++)
      {
      if(cx > localRect.right())
        {
        break;
        }

      logical = this->HeaderView->logicalIndex(i);
      cx += this->HeaderView->sectionSize(logical);
      if(cx >= localRect.left())
        {
        columnList.append(logical);
        }
      }
    }

  if(this->selectionBehavior() == QAbstractItemView::SelectColumns)
    {
    // Select the columns in the rectangle.
    item = this->getNextVisibleItem(this->Root);
    for(iter = columnList.begin(); iter != columnList.end(); ++iter)
      {
      index = item->Index;
      if(*iter != 0)
        {
        index = index.sibling(index.row(), *iter);
        }

      if(this->model()->flags(index) &= Qt::ItemIsSelectable)
        {
        selection.select(index, index);
        }
      }
    }
  else
    {
    // Search for the visible items in the rectangle.
    item = this->getNextVisibleItem(this->Root);
    while(item)
      {
      if(item->ContentsY + this->ItemHeight >= localRect.top())
        {
        // Break out of the loop when the items' y coordinate is larger
        // than the bottom of the rectangle.
        if(item->ContentsY > localRect.bottom())
          {
          break;
          }

        if(this->selectionBehavior() == QAbstractItemView::SelectRows)
          {
          // If selecting the row, add the column 0 index to the
          // selection. Make sure the index is selectable.
          if(this->model()->flags(item->Index) &= Qt::ItemIsSelectable)
            {
            selection.select(item->Index, item->Index);
            }
          }
        else // SelectItems
          {
          // Select the columns inside the rectangle. Only add the
          // index if it is selectable.
          for(iter = columnList.begin(); iter != columnList.end(); ++iter)
            {
            index = item->Index;
            if(*iter != 0)
              {
              index = index.sibling(index.row(), *iter);
              }

            if(this->model()->flags(index) &= Qt::ItemIsSelectable)
              {
              selection.select(index, index);
              }
            }
          }
        }

      item = this->getNextVisibleItem(item);
      }
    }

  this->selectionModel()->select(selection, command);
}

QRegion pqFlatTreeView::visualRegionForSelection(
    const QItemSelection &selection) const
{
  QRegion total;
  if(this->Root && this->HeaderView)
    {
    // Loop through the list of selection ranges to sum up the
    // bounding rectangles.
    QRect topLeft;
    QRect bottomRight;
    QItemSelection::ConstIterator iter = selection.begin();
    for( ; iter != selection.end(); ++iter)
      {
      topLeft = this->visualRect((*iter).topLeft());
      bottomRight = this->visualRect((*iter).bottomRight());
      if(topLeft.isValid() && bottomRight.isValid())
        {
        topLeft.setBottomRight(bottomRight.bottomRight());
        total.unite(QRegion(topLeft));
        }
      }
    }

  return total;
}

int pqFlatTreeView::horizontalOffset() const
{
  return this->horizontalScrollBar()->value();
}

int pqFlatTreeView::verticalOffset() const
{
  return this->verticalScrollBar()->value();
}

void pqFlatTreeView::resizeEvent(QResizeEvent *e)
{
  // Resize the header to the viewport width.
  if(e && this->HeaderView)
    {
    QSize hsize = this->HeaderView->sizeHint();
    hsize.setWidth(e->size().width());
    this->HeaderView->resize(hsize);

    // Set the scrollbar page size based on the new size. Update
    // the scrollbar maximums based on the new size and the contents
    // size.
    this->verticalScrollBar()->setPageStep(e->size().height());
    this->horizontalScrollBar()->setPageStep(e->size().width());
    this->updateScrollBars();
    }

  QAbstractItemView::resizeEvent(e);
}

bool pqFlatTreeView::viewportEvent(QEvent *e)
{
  if(e->type() == QEvent::FontChange)
    {
    // Layout all the items for the new font.
    this->FontChanged = true;
    this->layoutItems();
    this->viewport()->update();
    }

  return QAbstractItemView::viewportEvent(e);
}

void pqFlatTreeView::paintEvent(QPaintEvent *e)
{
  if(!e || !this->Root || !this->HeaderView)
    {
    return;
    }

  QPainter painter(this->viewport());
  if(!painter.isActive())
    {
    return;
    }

  // Translate the painter and paint area to contents coordinates.
  int px = this->horizontalOffset();
  int py = this->verticalOffset();
  painter.translate(QPoint(-px, -py));
  QRect area = e->rect();
  area.translate(QPoint(px, py));

  // Setup the view options.
  int i = 0;
  QStyleOptionViewItem options = this->viewOptions();
  int fontHeight = options.fontMetrics.height();
  int fontAscent = options.fontMetrics.ascent();
  painter.setFont(options.font);
  QColor branchColor(Qt::darkGray);
  QColor expandColor(Qt::black);

  // Draw the column selection first so it is behind the text.
  int columnWidth = 0;
  if(this->Internal)
    {
    int totalHeight = this->viewport()->height();
    if(this->ContentsHeight > totalHeight)
      {
      totalHeight = this->ContentsHeight;
      }

    for(i = 0; i < this->Internal->size(); i++)
      {
      if((*this->Internal)[i]->Selected)
        {
        px = this->HeaderView->sectionPosition(i);
        columnWidth = this->HeaderView->sectionSize(i);
        painter.fillRect(px, 0, columnWidth, totalHeight,
            options.palette.highlight());
        }
      }
    }

  // Loop through the visible items. Paint the items that are in the
  // repaint area.
  QModelIndex index;
  QAbstractItemModel *itemModel = this->model();
  int columns = itemModel->columnCount(this->Root->Index);
  int halfIndent = this->IndentWidth / 2;
  int trueHeight = this->ItemHeight - pqFlatTreeView::PipeLength;
  int elideLength = options.fontMetrics.width("...");
  pqFlatTreeViewItem *item = this->getNextVisibleItem(this->Root);
  while(item)
    {
    if(item->ContentsY + this->ItemHeight >= area.top())
      {
      if(item->ContentsY <= area.bottom())
        {
        // If the row is highlighted, draw the background highlight
        // before the data.
        px = 0;
        int itemWidth = 0;
        if(item->RowSelected)
          {
          itemWidth = this->viewport()->width();
          if(this->ContentsWidth > itemWidth)
            {
            itemWidth = this->ContentsWidth;
            }

          painter.fillRect(0, item->ContentsY + pqFlatTreeView::PipeLength,
              itemWidth, trueHeight, options.palette.highlight());
          }

        for(i = 0; i < columns; i++)
          {
          // The columns can be moved by the header. Make sure they are
          // drawn in the correct location.
          px = this->HeaderView->sectionPosition(i);
          columnWidth = this->HeaderView->sectionSize(i) -
              pqFlatTreeView::TextMargin;

          // TODO: Clip text drawing to column width.

          // Column 0 has the icon and indent. We may want to support
          // decoration for other columns in the future.
          QIcon icon;
          QPixmap pixmap;
          if(i == 0)
            {
            // If the item is selected and the decoration should be
            // highlighted, draw a highlight behind the banches.
            if(options.showDecorationSelected && item->Cells[i]->Selected)
              {
              painter.fillRect(px, item->ContentsY + pqFlatTreeView::PipeLength,
                  item->Indent, trueHeight, options.palette.highlight());
              }

            px += item->Indent;
            columnWidth -= item->Indent;

            // Get the icon for this item from the model. QVariant won't
            // automatically convert from pixmap to icon, so it has to
            // be done here.
            QVariant decoration = itemModel->data(item->Index,
                Qt::DecorationRole);
            if(decoration.canConvert(QVariant::Pixmap))
              {
              icon = qvariant_cast<QPixmap>(decoration);
              }
            else if(decoration.canConvert(QVariant::Icon))
              {
              icon = qvariant_cast<QIcon>(decoration);
              }

            if(!icon.isNull() && columnWidth > 0)
              {
              // If the item is selected and the decoration should be
              // highlighted, draw a highlight behind the icon.
              itemWidth = this->IndentWidth + pqFlatTreeView::TextMargin;
              py = item->ContentsY + pqFlatTreeView::PipeLength;
              if(options.showDecorationSelected && item->Cells[i]->Selected)
                {
                painter.fillRect(px, py, itemWidth, trueHeight,
                    options.palette.highlight());
                }

              // Draw the decoration icon. Scale the icon to the size in
              // the style options. The icon will always be centered in x
              // since the width is based on the icon size. The style
              // option should be followed for the y placement.
              if(options.decorationAlignment & Qt::AlignVCenter)
                {
                py += (trueHeight - this->IndentWidth) / 2;
                }
              else if(options.decorationAlignment & Qt::AlignBottom)
                {
                py += trueHeight - this->IndentWidth;
                }

              pixmap = icon.pixmap(options.decorationSize);
              painter.drawPixmap(px + 1, py + 1, pixmap);

              // Add extra padding to the x coordinate to put space
              // between the icon and the text.
              px += itemWidth;
              columnWidth -= itemWidth;
              }
            }

          // Draw in the display data.
          if(i == 0)
            {
            index = item->Index;
            }
          else
            {
            index = item->Index.sibling(item->Index.row(), i);
            }

          QVariant indexData = itemModel->data(index);
          py = item->ContentsY + pqFlatTreeView::PipeLength;
          if(indexData.type() == QVariant::Pixmap)
            {
            pixmap = qvariant_cast<QPixmap>(indexData);
            if(pixmap.height() > trueHeight)
              {
              pixmap = pixmap.scaledToHeight(trueHeight);
              }
            else if(options.displayAlignment & Qt::AlignVCenter)
              {
              py += (trueHeight - pixmap.height()) / 2;
              }
            else if(options.displayAlignment & Qt::AlignBottom)
              {
              py += trueHeight - pixmap.height();
              }

            painter.drawPixmap(px, py, pixmap);
            }
          else if(indexData.canConvert(QVariant::Icon))
            {
            icon = qvariant_cast<QIcon>(indexData);
            pixmap = icon.pixmap(options.decorationSize);

            // Adjust the vertical text alignment according to the style.
            if(options.displayAlignment & Qt::AlignVCenter)
              {
              py += (trueHeight - pixmap.height()) / 2;
              }
            else if(options.displayAlignment & Qt::AlignBottom)
              {
              py += trueHeight - pixmap.height();
              }

            painter.drawPixmap(px, py, pixmap);
            }
          else
            {
            QString text = indexData.toString();
            if(!text.isEmpty() && columnWidth > 0)
              {
              // Set the text color based on the highlighted state.
              if(item->RowSelected || (*this->Internal)[i]->Selected ||
                  item->Cells[i]->Selected)
                {
                // If the item is selected, draw the background highlight
                // before drawing the text.
                if(item->Cells[i]->Selected)
                  {
                  itemWidth = item->Cells[i]->Width;
                  if(itemWidth > columnWidth)
                    {
                    itemWidth = columnWidth;
                    }

                  painter.fillRect(px, py, itemWidth, trueHeight,
                      options.palette.highlight());
                  }

                painter.setPen(options.palette.color(QPalette::Normal,
                    QPalette::HighlightedText));
                }
              else
                {
                painter.setPen(options.palette.color(QPalette::Normal,
                    QPalette::Text));
                }

              // Adjust the vertical text alignment according to the style.
              if(options.displayAlignment & Qt::AlignVCenter)
                {
                py += (trueHeight - fontHeight) / 2;
                }
              else if(options.displayAlignment & Qt::AlignBottom)
                {
                py += trueHeight - fontHeight;
                }

              // If the text is too wide for the column, adjust the text
              // so it fits. Use the text elide style from the options.
              if(item->Cells[i]->Width > columnWidth)
                {
                int length = (text.length() * (columnWidth - elideLength)) /
                    item->Cells[i]->Width;
                if(length < 1)
                  {
                  length = 1;
                  }

                if(options.textElideMode == Qt::ElideRight)
                  {
                  text = text.left(length) + "...";
                  }
                else if(options.textElideMode == Qt::ElideLeft)
                  {
                  text = "..." + text.right(length);
                  }
                else // ElideMiddle
                  {
                  int leftHalf = length / 2;
                  int rightHalf = leftHalf;
                  if(leftHalf + rightHalf != length)
                    {
                    leftHalf++;
                    }

                  text = text.left(leftHalf) + "..." + text.right(rightHalf);
                  }
                }

              painter.drawText(px, py + fontAscent, text);
              }
            }

          if(i == 0)
            {
            this->drawBranches(painter, item, halfIndent, branchColor,
                expandColor, options);
            }

          // If this is the current index, draw the focus rectangle.
          if(this->selectionBehavior() == QAbstractItemView::SelectItems &&
              index == this->selectionModel()->currentIndex())
            {
            QStyleOptionFocusRect opt;
            opt.QStyleOption::operator=(options);
            if(item->Cells[i]->Selected)
              {
              opt.backgroundColor = options.palette.color(QPalette::Normal,
                  QPalette::Highlight);
              }
            else
              {
              opt.backgroundColor = options.palette.color(QPalette::Normal,
                  QPalette::Base);
              }

            opt.state |= QStyle::State_KeyboardFocusChange;
            opt.state |= QStyle::State_HasFocus;
            itemWidth = item->Cells[i]->Width;
            if(itemWidth > columnWidth)
              {
              itemWidth = columnWidth;
              }

            if(options.showDecorationSelected)
              {
              // TODO
              }

            opt.rect.setRect(px, item->ContentsY + pqFlatTreeView::PipeLength,
                itemWidth, trueHeight);
            QApplication::style()->drawPrimitive(QStyle::PE_FrameFocusRect,
                &opt, &painter);
            }
          }

        // If this is the current row, draw the current outline.
        if(this->selectionBehavior() == QAbstractItemView::SelectRows)
          {
          index = this->selectionModel()->currentIndex();
          if(index.isValid() && index.row() == item->Index.row() &&
              index.parent() == item->Index.parent())
            {
            QStyleOptionFocusRect opt;
            opt.QStyleOption::operator=(options);
            if(item->RowSelected)
              {
              opt.backgroundColor = options.palette.color(QPalette::Normal,
                  QPalette::Highlight);
              }
            else
              {
              opt.backgroundColor = options.palette.color(QPalette::Normal,
                  QPalette::Base);
              }

            opt.state |= QStyle::State_KeyboardFocusChange;
            opt.state |= QStyle::State_HasFocus;

            itemWidth = this->viewport()->width();
            if(this->ContentsWidth > itemWidth)
              {
              itemWidth = this->ContentsWidth;
              }

            opt.rect.setRect(0, item->ContentsY + pqFlatTreeView::PipeLength,
                itemWidth, trueHeight);
            QApplication::style()->drawPrimitive(QStyle::PE_FrameFocusRect,
                &opt, &painter);
            }
          }
        }
      else
        {
        break;
        }
      }

    item = this->getNextVisibleItem(item);
    }

  // TODO: If using column selection, draw the current column outline.
  if(this->selectionBehavior() == QAbstractItemView::SelectColumns)
  {
    index = this->selectionModel()->currentIndex();
    if(index.isValid())
      {
      }
  }
}

void pqFlatTreeView::handleSectionResized(int, int, int)
{
  if(!this->InUpdateWidth && this->HeaderView)
    {
    // Update the contents width if the user changed the size.
    this->ManageSizes = false;
    this->updateContentsWidth();
    this->updateScrollBars();

    // Repaint the viewport to show the changes.
    this->viewport()->update();
    }
}

void pqFlatTreeView::handleSectionMoved(int, int, int)
{
  // Repaint the viewport to show the changes.
  this->viewport()->update();
}

void pqFlatTreeView::changeCurrent(const QModelIndex &current,
    const QModelIndex &previous)
{
  if(this->selectionBehavior() == QAbstractItemView::SelectItems)
    {
    // Deselect the previous item.
    QRegion region;
    pqFlatTreeViewItem *item = 0;
    if(previous.isValid())
      {
      item = this->getItem(previous);
      if(item && previous.column() < item->Cells.size())
        {
        region = QRegion(0, item->ContentsY, this->ContentsWidth,
            this->ItemHeight);
        }
      }

    // Select the new item.
    if(current.isValid())
      {
      item = this->getItem(current);
      if(item && current.column() < item->Cells.size())
        {
        region = region.unite(QRegion(0, item->ContentsY, this->ContentsWidth,
            this->ItemHeight));
        }
      }

    // Repaint the affected region.
    if(!region.isEmpty())
      {
      region.translate(-this->horizontalOffset(), -this->verticalOffset());
      this->viewport()->update(region);
      }
    }
}

void pqFlatTreeView::changeCurrentRow(const QModelIndex &current,
    const QModelIndex &previous)
{
  if(this->selectionBehavior() == QAbstractItemView::SelectRows)
    {
    // Deselect the previous item.
    QRegion region;
    pqFlatTreeViewItem *item = 0;
    if(previous.isValid())
      {
      item = this->getItem(previous);
      if(item)
        {
        region = QRegion(0, item->ContentsY, this->ContentsWidth,
            this->ItemHeight);
        }
      }

    // Select the new item.
    if(current.isValid())
      {
      item = this->getItem(current);
      if(item)
        {
        region = region.unite(QRegion(0, item->ContentsY, this->ContentsWidth,
            this->ItemHeight));
        }
      }

    // Repaint the affected region.
    if(!region.isEmpty())
      {
      region.translate(-this->horizontalOffset(), -this->verticalOffset());
      this->viewport()->update(region);
      }
    }
}

void pqFlatTreeView::changeCurrentColumn(const QModelIndex &,
    const QModelIndex &)
{
  if(this->selectionBehavior() == QAbstractItemView::SelectColumns)
    {
    this->viewport()->update();
    }
}

void pqFlatTreeView::changeSelection(const QItemSelection &selected,
    const QItemSelection &deselected)
{
  if(!this->Root || !this->HeaderView || !this->Internal)
    {
    return;
    }

  QRegion region;
  int start = 0;
  int end = 0;
  int column = 0;
  int i = 0;
  int cy = 0;
  int totalHeight = 0;
  int totalWidth = this->viewport()->width();
  if(totalWidth < this->ContentsWidth)
    {
    totalWidth = this->ContentsWidth;
    }

  // Start with the deselected items.
  pqFlatTreeViewItem *parentItem = 0;
  pqFlatTreeViewItem *item = 0;
  QItemSelection::ConstIterator iter = deselected.begin();
  for( ; iter != deselected.end(); ++iter)
    {
    if(!(*iter).isValid())
      {
      continue;
      }

    // Get the parent item for the range.
    parentItem = this->getItem((*iter).parent());
    if(parentItem)
      {
      if(this->selectionBehavior() == QAbstractItemView::SelectColumns)
        {
        end = (*iter).right();
        for(start = (*iter).left(); start <= end; start++)
          {
          (*this->Internal)[start]->Selected = false;
          }
        }
      else if(parentItem->Items.size() > 0)
        {
        cy = -1;
        start = (*iter).top();
        end = (*iter).bottom();
        if(end >= parentItem->Items.size())
          {
          end = parentItem->Items.size() - 1;
          }

        for( ; start <= end; start++)
          {
          item = parentItem->Items[start];
          if(cy == -1)
            {
            cy = item->ContentsY;
            }

          if(this->selectionBehavior() == QAbstractItemView::SelectRows)
            {
            item->RowSelected = false;
            }
          else
            {
            i = (*iter).left();
            column = (*iter).right();
            for( ; i <= column; i++)
              {
              item->Cells[i]->Selected = false;
              }
            }
          }

        // Add the affected area to the repaint list.
        totalHeight = (*iter).height() * this->ItemHeight;
        region = region.unite(QRegion(0, cy, totalWidth, totalHeight));
        }
      }
    }

  // Mark the newly selected items, so they will get highlighted.
  for(iter = selected.begin(); iter != selected.end(); ++iter)
    {
    if(!(*iter).isValid())
      {
      continue;
      }

    parentItem = this->getItem((*iter).parent());
    if(parentItem)
      {
      if(this->selectionBehavior() == QAbstractItemView::SelectColumns)
        {
        end = (*iter).right();
        for(start = (*iter).left(); start <= end; start++)
          {
          (*this->Internal)[start]->Selected = true;
          }
        }
      else if(parentItem->Items.size() > 0)
        {
        cy = -1;
        start = (*iter).top();
        end = (*iter).bottom();
        if(end >= parentItem->Items.size())
          {
          end = parentItem->Items.size() - 1;
          }

        for( ; start <= end; start++)
          {
          item = parentItem->Items[start];
          if(cy == -1)
            {
            cy = item->ContentsY;
            }

          if(this->selectionBehavior() == QAbstractItemView::SelectRows)
            {
            item->RowSelected = true;
            }
          else
            {
            i = (*iter).left();
            column = (*iter).right();
            for( ; i <= column && i < item->Cells.size(); i++)
              {
              item->Cells[i]->Selected = true;
              }
            }
          }

        // Add the affected area to the repaint list.
        totalHeight = (*iter).height() * this->ItemHeight;
        region = region.unite(QRegion(0, cy, totalWidth, totalHeight));
        }
      }
    }

  if(this->selectionBehavior() == QAbstractItemView::SelectColumns &&
      (selected.size() > 0 || deselected.size() > 0))
    {
    this->viewport()->update();
    }
  else if(!region.isEmpty())
    {
    region.translate(-this->horizontalOffset(), -this->verticalOffset());
    this->viewport()->update(region);
    }
}

void pqFlatTreeView::resetInternal(int numberOfColumns)
{
  if(!this->Internal)
    {
    this->Internal = new pqFlatTreeViewInternal();
    }

  if(this->Internal)
    {
    this->Internal->clear();
    for(int i = 0; i < numberOfColumns; i++)
      {
      this->Internal->append(new pqFlatTreeViewColumn());
      }
    }
}

void pqFlatTreeView::layoutItems()
{
  if(this->Root && this->HeaderView)
    {
    // Determine the item height based on the font and icon size.
    // The minimum indent width should be 18 to fit the +/- icon.
    QStyleOptionViewItem options = this->viewOptions();
    this->IndentWidth = options.decorationSize.height() + 2;
    this->ItemHeight = options.fontMetrics.height();
    if(this->IndentWidth < 18)
      {
      this->IndentWidth = 18;
      }
    if(this->IndentWidth > this->ItemHeight)
      {
      this->ItemHeight = this->IndentWidth;
      }

    // Add padding to the height for the vertical connection.
    this->ItemHeight += pqFlatTreeView::PipeLength;

    // If the header is shown, adjust the starting point of the
    // item layout.
    int point = 0;
    if(!this->HeaderView->isHidden())
      {
      point = this->HeaderView->size().height();
      }

    // Reset the preferred size array.
    this->resetInternal(this->model()->columnCount(this->Root->Index));

    // Loop through all the items to set up their bounds.
    pqFlatTreeViewItem *item = this->getNextVisibleItem(this->Root);
    while(item)
      {
      this->layoutItem(item, point, options.fontMetrics);
      item = this->getNextVisibleItem(item);
      }

    // Update the contents size. Adjust the scrollbars for the new
    // item height and contents size.
    this->ContentsHeight = point;
    this->updateContentsWidth();
    this->verticalScrollBar()->setSingleStep(this->ItemHeight);
    this->horizontalScrollBar()->setSingleStep(this->IndentWidth);
    this->updateScrollBars();
    }

  // Clear the font changed flag.
  this->FontChanged = false;
}

void pqFlatTreeView::layoutItem(pqFlatTreeViewItem *item, int &point,
    const QFontMetrics &fm)
{
  if(item)
    {
    // Set up the bounds for the item. Increment the starting point
    // for the next item.
    item->ContentsY = point;
    point += this->ItemHeight;

    // The indent is based on the parent indent. If the parent has
    // more than one child, increase the indent.
    item->Indent = item->Parent->Indent;
    if(item->Parent->Items.size() > 1)
      {
      item->Indent += this->IndentWidth;
      }

    // Make sure the text width list is allocated.
    int i = 0;
    if(this->Internal && item->Cells.size() == 0)
      {
      for(i = 0; i < this->Internal->size(); i++)
        {
        item->Cells.append(new pqFlatTreeViewColumn);
        }
      }

    // The indent may change causing the desired width to change.
    int preferredWidth = 0;
    if(item->Cells.size() > 0)
      {
      if(item->Cells[0]->Width == 0 || this->FontChanged)
        {
        item->Cells[0]->Width = this->getDataWidth(item->Index, fm);
        }

      // The text width, the indent, the icon width, and the padding
      // between the icon and the item all factor into the desired width.
      preferredWidth = item->Cells[0]->Width + item->Indent +
          this->IndentWidth + pqFlatTreeView::DoubleTextMargin;
      if(preferredWidth > (*this->Internal)[0]->Width)
        {
        (*this->Internal)[0]->Width = preferredWidth;
        }
      }

    for(i = 1; i < item->Cells.size(); i++)
      {
      if(item->Cells[i]->Width == 0 || this->FontChanged)
        {
        // Get the data from the model. If the data is a string, use
        // the font metrics to determine the desired width. If the
        // item is an image or list of images, the desired width is
        // the image width(s).
        QModelIndex index = item->Index.sibling(item->Index.row(), i);
        item->Cells[i]->Width = this->getDataWidth(index, fm);
        preferredWidth = item->Cells[i]->Width + pqFlatTreeView::TextMargin;
        if(preferredWidth > (*this->Internal)[i]->Width)
          {
          (*this->Internal)[i]->Width = preferredWidth;
          }
        }
      }
    }
}

int pqFlatTreeView::getDataWidth(const QModelIndex &index,
    const QFontMetrics &fm) const
{
  QVariant indexData = index.data();
  if(indexData.type() == QVariant::Pixmap)
    {
    // Make sure the pixmap is scaled to fit the uniform item height.
    QSize pixmapSize = qvariant_cast<QPixmap>(indexData).size();
    int allowed = this->ItemHeight - pqFlatTreeView::PipeLength;
    if(pixmapSize.height() > allowed)
      {
      pixmapSize.scale(pixmapSize.width(), allowed, Qt::KeepAspectRatio);
      }

    return pixmapSize.width();
    }
  else if(indexData.canConvert(QVariant::Icon))
    {
    // Icons will be scaled to fit the style options.
    return this->viewOptions().decorationSize.width();
    }
  else
    {
    // Find the font width for the string.
    return fm.width(indexData.toString());
    }
}

bool pqFlatTreeView::updateContentsWidth()
{
  bool sectionSizeChanged = false;
  int oldContentsWidth = this->ContentsWidth;
  this->ContentsWidth = 0;
  if(this->HeaderView)
    {
    // Manage the header section sizes if the header is not visible.
    if(this->Internal && (this->ManageSizes || this->HeaderView->isHidden()))
      {
      this->InUpdateWidth = true;
      int newWidth = 0;
      int oldWidth = 0;
      for(int i = 0; i < this->Internal->size(); i++)
        {
        oldWidth = this->HeaderView->sectionSize(i);
        newWidth = this->HeaderView->sectionSizeHint(i);
        if(newWidth < (*this->Internal)[i]->Width)
          {
          newWidth = (*this->Internal)[i]->Width;
          }

        if(newWidth != oldWidth)
          {
          this->HeaderView->resizeSection(i, newWidth);
          sectionSizeChanged = true;
          }
        }

      this->InUpdateWidth = false;
      }

    this->ContentsWidth = this->HeaderView->length();
    }

  return sectionSizeChanged || this->ContentsWidth != oldContentsWidth;
}

void pqFlatTreeView::updateScrollBars()
{
  int value = this->ContentsHeight - this->viewport()->height();
  this->verticalScrollBar()->setMaximum(value < 0 ? 0 : value);
  value = this->ContentsWidth - this->viewport()->width();
  this->horizontalScrollBar()->setMaximum(value < 0 ? 0 : value);
}

void pqFlatTreeView::addChildItems(pqFlatTreeViewItem *item,
    int parentChildCount)
{
  if(item)
    {
    // Get the number of children from the model. The model may
    // delay loading information. Force the model to load the
    // child information if the item can't be made expandable.
    QAbstractItemModel *itemModel = this->model();
    if(itemModel->canFetchMore(item->Index))
      {
      // An item can be expandable only if the parent has more
      // than one child item.
      if(parentChildCount > 1 && !item->Expanded)
        {
        item->Expandable = true;
        return;
        }
      else
        {
        itemModel->fetchMore(item->Index);
        }
      }

    int count = itemModel->rowCount(item->Index);
    item->Expandable = parentChildCount > 1 && count > 0;
    if(item->Expanded || !item->Expandable)
      {
      // Set up the parent and model index for each added child.
      // The model's hierarchical data should be in column 0.
      QModelIndex index;
      pqFlatTreeViewItem *child = 0;
      for(int i = 0; i < count; i++)
        {
        index = itemModel->index(i, 0, item->Index);
        if(index.isValid())
          {
          child = new pqFlatTreeViewItem();
          if(child)
            {
            child->Parent = item;
            child->Index = index;
            item->Items.append(child);
            this->addChildItems(child, count);
            }
          }
        }
      }
    }
}

bool pqFlatTreeView::getIndexRowList(const QModelIndex &index,
    pqFlatTreeViewItemRows &rowList) const
{
  // Make sure the index is for the current model. If the index is
  // invalid, it refers to the root. The model won't be set in that
  // case. If the index is valid, the model can be checked.
  if(index.isValid() && index.model() != this->model())
    {
    return false;
    }

  if(!this->Root)
    {
    return false;
    }

  // Get the row hierarchy from the index and its ancestors.
  // Make sure the index is for column 0.
  QModelIndex tempIndex = index;
  if(index.isValid() && index.column() > 0)
    {
    tempIndex = index.sibling(index.row(), 0);
    }

  while(tempIndex.isValid() && tempIndex != this->Root->Index)
    {
    rowList.prepend(tempIndex.row());
    tempIndex = tempIndex.parent();
    }

  // Return false if the root was not found in the ancestry.
  return tempIndex == this->Root->Index;
}

pqFlatTreeViewItem *pqFlatTreeView::getItem(
    const pqFlatTreeViewItemRows &rowList) const
{
  pqFlatTreeViewItem *item = this->Root;
  pqFlatTreeViewItemRows::ConstIterator iter = rowList.begin();
  for( ; iter != rowList.end(); ++iter)
    {
    if(*iter >= 0 && *iter < item->Items.size())
      {
      item = item->Items[*iter];
      }
    else
      {
      return 0;
      }
    }

  return item;
}

pqFlatTreeViewItem *pqFlatTreeView::getItem(const QModelIndex &index) const
{
  pqFlatTreeViewItem *item = 0;
  pqFlatTreeViewItemRows rowList;
  if(this->getIndexRowList(index, rowList))
    {
    item = this->getItem(rowList);
    }

  return item;
}

pqFlatTreeViewItem *pqFlatTreeView::getNextVisibleItem(
    pqFlatTreeViewItem *item) const
{
  if(item)
    {
    if(item->Expandable)
      {
      if(item->Expanded)
        {
        return item->Items[0];
        }
      }
    else if(item->Items.size() > 0)
      {
      return item->Items[0];
      }

    // Search up the ancestors for an item with multiple children.
    // The next item will be the next child.
    int row = 0;
    int count = 0;
    while(item->Parent)
      {
      count = item->Parent->Items.size();
      if(count > 1)
        {
        row = item->Parent->Items.indexOf(item) + 1;
        if(row < count)
          {
          return item->Parent->Items[row];
          }
        }

      item = item->Parent;
      }
    }

  return 0;
}

pqFlatTreeViewItem *pqFlatTreeView::getPreviousVisibleItem(
    pqFlatTreeViewItem *item) const
{
  if(item && item->Parent && item->Parent != this->Root)
    {
    int row = item->Parent->Items.indexOf(item);
    if(row == 0)
      {
      return item->Parent;
      }
    else
      {
      item = item->Parent->Items[row - 1];
      while(item->Items.size() > 0)
        {
        item = item->Items.last();
        }

      return item;
      }
    }

  return 0;
}

pqFlatTreeViewItem *pqFlatTreeView::getLastVisibleItem() const
{
  if(this->Root && this->Root->Items.size() > 0)
    {
    pqFlatTreeViewItem *item = this->Root->Items.last();
    while(item->Items.size() > 0)
      {
      item = item->Items.last();
      }

    return item;
    }

  return 0;
}

void pqFlatTreeView::drawBranches(QPainter &painter, pqFlatTreeViewItem *item,
    int halfIndent, const QColor &branchColor, const QColor &expandColor,
    QStyleOptionViewItem &options)
{
  // Draw the tree branches for the row. First draw the tree
  // branch for the current item. Then, step back up the parent
  // chain to draw the branches that pass through this row.
  int px = this->HeaderView->sectionPosition(0) + item->Indent;
  int py = 0;
  painter.setPen(branchColor);
  if(item->Parent->Items.size() > 1)
    {
    px -= halfIndent;
    py = item->ContentsY + pqFlatTreeView::PipeLength + halfIndent;
    int endY = py;
    if(item != item->Parent->Items.last())
      {
      endY = item->ContentsY + this->ItemHeight;
      }

    painter.drawLine(px, py, px + halfIndent - 1, py);
    painter.drawLine(px, item->ContentsY, px, endY);
    if(item->Expandable)
      {
      // If the item is expandable, draw in the little box with
      // the +/- icon.
      painter.fillRect(px - 4, py - 4, 8, 8, options.palette.base());
      painter.drawRect(px - 4, py - 4, 8, 8);

      painter.setPen(expandColor);
      painter.drawLine(px - 2, py, px + 2, py);
      if(!item->Expanded)
        {
        painter.drawLine(px, py - 2, px, py + 2);
        }
      painter.setPen(branchColor);
      }
    }
  else
    {
    px += halfIndent;
    painter.drawLine(px, item->ContentsY, px, item->ContentsY +
        pqFlatTreeView::PipeLength);
    }

  py = item->ContentsY + this->ItemHeight;
  pqFlatTreeViewItem *branchItem = item->Parent;
  while(branchItem->Parent)
    {
    // Search for the next parent with more than one child.
    while(branchItem->Parent && branchItem->Parent->Items.size() < 2)
      {
      branchItem = branchItem->Parent;
      }

    if(!branchItem->Parent)
      {
      break;
      }

    // If the item is not last in the parent's children, draw a
    // branch passing through the item.
    px -= this->IndentWidth;
    if(branchItem != branchItem->Parent->Items.last())
      {
      painter.drawLine(px, item->ContentsY, px, py);
      }

    branchItem = branchItem->Parent;
    }
}


