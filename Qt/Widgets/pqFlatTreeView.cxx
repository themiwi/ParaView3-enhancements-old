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

#include <QAbstractItemModel>
#include <QApplication>
#include <QEvent>
#include <QHeaderView>
#include <QItemEditorFactory>
#include <QLineEdit>
#include <QList>
#include <QPainter>
#include <QPaintEvent>
#include <QPersistentModelIndex>
#include <QPoint>
#include <QRect>
#include <QScrollBar>
#include <QStyle>
#include <QtDebug>


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

class pqFlatTreeViewInternal
{
public:
  pqFlatTreeViewInternal();
  ~pqFlatTreeViewInternal() {}

  QPersistentModelIndex ShiftStart;
  QPersistentModelIndex Index;
  QWidget *Editor;
};


//----------------------------------------------------------------------------
pqFlatTreeViewColumn::pqFlatTreeViewColumn()
{
  this->Width = 0;
  this->Selected = false;
}


//----------------------------------------------------------------------------
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


//----------------------------------------------------------------------------
pqFlatTreeViewInternal::pqFlatTreeViewInternal()
  : ShiftStart(), Index()
{
  this->Editor = 0;
}


//----------------------------------------------------------------------------
int pqFlatTreeView::TextMargin = 4;
int pqFlatTreeView::DoubleTextMargin = 2 * pqFlatTreeView::TextMargin;
int pqFlatTreeView::PipeLength = 4;

pqFlatTreeView::pqFlatTreeView(QWidget *p)
  : QAbstractScrollArea(p)
{
  this->Model = 0;
  this->Selection = 0;
  this->Behavior = pqFlatTreeView::SelectItems;
  this->Mode = pqFlatTreeView::SingleSelection;
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
  this->HeaderOwned = false;
  this->SelectionOwned = false;

  // Set up the default header view.
  this->setHeader(0);
}

pqFlatTreeView::~pqFlatTreeView()
{
  delete this->Root;
  delete this->Internal;
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

      QFontMetrics fm = this->fontMetrics();
      pqFlatTreeViewItem *item = this->getNextVisibleItem(this->Root);
      while(item)
        {
        this->layoutItem(item, point, fm);
        item = this->getNextVisibleItem(item);
        }

      // Update the contents size and repaint the viewport.
      this->ContentsHeight = point;
      this->updateContentsWidth();
      this->updateScrollBars();
      this->layoutEditor();
      this->viewport()->update();
      }
    }
  else if(object && object == this->Internal->Editor)
    {
    if(e->type() == QEvent::KeyPress)
      {
      int key = static_cast<QKeyEvent *>(e)->key();
      if(key == Qt::Key_Enter || key == Qt::Key_Return)
        {
        this->finishEditing();
        this->viewport()->setFocus();
        return true;
        }
      else if(key == Qt::Key_Escape)
        {
        this->cancelEditing();
        this->viewport()->setFocus();
        return true;
        }
      }
    else if(e->type() == QEvent::FocusOut)
      {
      bool finishNeeded = true;
      QWidget *widget = QApplication::focusWidget();
      while(widget)
        {
        // Don't worry about focus changes internally in the editor.
        if(widget == this->Internal->Editor)
          {
          finishNeeded = false;
          break;
          }

        widget = widget->parentWidget();
        }

      if(finishNeeded)
        {
        this->finishEditing();
        return true;
        }
      }
    }

  return QAbstractScrollArea::eventFilter(object, e);
}

void pqFlatTreeView::setModel(QAbstractItemModel *model)
{
  if(model == this->Model)
    {
    return;
    }

  if(this->Model)
    {
    // Disconnect from the previous model's signals.
    this->disconnect(this->Model, 0, this, 0);
    }

  if(this->Selection)
    {
    // Disconnect from the selection model's signals.
    this->disconnect(this->Selection, 0, this, 0);
    this->Internal->ShiftStart = QPersistentModelIndex();
    }

  this->cancelEditing();
  this->resetRoot();
  this->Model = model;
  if(this->Model)
    {
    // Listen for model changes.
    this->connect(this->Model, SIGNAL(modelReset()), this, SLOT(reset()));
    this->connect(this->Model, SIGNAL(layoutChanged()), this, SLOT(reset()));
    this->connect(this->Model,
        SIGNAL(rowsInserted(const QModelIndex &, int, int)),
        this, SLOT(insertRows(const QModelIndex &, int, int)));
    this->connect(this->Model,
        SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
        this, SLOT(startRowRemoval(const QModelIndex &, int, int)));
    this->connect(this->Model,
        SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
        this, SLOT(finishRowRemoval(const QModelIndex &, int, int)));
    this->connect(this->Model,
        SIGNAL(columnsInserted(const QModelIndex &, int, int)),
        this, SLOT(insertColumns(const QModelIndex &, int, int)));
    this->connect(this->Model,
        SIGNAL(columnsAboutToBeRemoved(const QModelIndex &, int, int)),
        this, SLOT(startColumnRemoval(const QModelIndex &, int, int)));
    this->connect(this->Model,
        SIGNAL(columnsRemoved(const QModelIndex &, int, int)),
        this, SLOT(finishColumnRemoval(const QModelIndex &, int, int)));
    this->connect(this->Model,
        SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
        this, SLOT(updateData(const QModelIndex &, const QModelIndex &)));
    }

  // Notify the header view of the model change.
  if(this->HeaderView)
    {
    this->HeaderView->setModel(this->Model);
    }

  // Create a new selection model for the new model.
  this->setSelectionModel(0);

  // Traverse the model to set up the view items. Only set up the
  // visible items.
  this->addChildItems(this->Root, 1);
  this->layoutItems();
  this->viewport()->update();
}

QModelIndex pqFlatTreeView::getRootIndex() const
{
  return QModelIndex(this->Root->Index);
}

void pqFlatTreeView::setRootIndex(const QModelIndex &index)
{
  // Make sure the index is for the current model.
  if(index.isValid() && index.model() != this->Model)
    {
    return;
    }

  if(this->Root->Index == index)
    {
    return;
    }

  this->cancelEditing();

  // Clean up the current view items. Assign the new root index.
  this->Internal->ShiftStart = QPersistentModelIndex();
  this->resetRoot();
  this->Root->Index = index;

  // Inform the header view of the root change.
  if(this->HeaderView)
    {
    this->HeaderView->setRootIndex(index);
    }

  // Traverse the model to set up the view items. Only set up the
  // visible items.
  this->addChildItems(this->Root, 1);
  this->layoutItems();
  this->changeSelection(this->Selection->selection(), QItemSelection());
  this->viewport()->update();
}

void pqFlatTreeView::setSelectionModel(QItemSelectionModel *selectionModel)
{
  // The selection model must reference the same model as the view.
  if(selectionModel && selectionModel->model() != this->Model)
    {
    return;
    }

  // If the default selection model is being set and is already in
  // use, don't do anything. Check the model to make sure the default
  // selection model is still valid.
  if(!selectionModel && this->Selection && this->SelectionOwned &&
      this->Selection->model() == this->Model)
    {
    }

  QItemSelectionModel *toDelete = 0;
  if(this->Selection)
    {
    // Disconnect from the selection model signals.
    this->disconnect(this->Selection, 0, this, 0);
    if(this->SelectionOwned)
      {
      this->SelectionOwned = false;
      toDelete = this->Selection;
      }

    // Clear all the selected flags in the view items.
    this->Internal->ShiftStart = QPersistentModelIndex();
    this->changeSelection(QItemSelection(), this->Selection->selection());
    }

  this->Selection = selectionModel;
  if(!this->Selection)
    {
    this->Selection = new QItemSelectionModel(this->Model, this);
    this->SelectionOwned = true;
    }

  // Listen to the selection model signals.
  this->connect(this->Selection,
      SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
      this, SLOT(changeCurrent(const QModelIndex &, const QModelIndex &)));
  this->connect(this->Selection,
      SIGNAL(currentRowChanged(const QModelIndex &, const QModelIndex &)),
      this,
      SLOT(changeCurrentRow(const QModelIndex &, const QModelIndex &)));
  this->connect(this->Selection,
      SIGNAL(currentColumnChanged(const QModelIndex &, const QModelIndex &)),
      this,
      SLOT(changeCurrentColumn(const QModelIndex &, const QModelIndex &)));
  this->connect(this->Selection,
      SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
      this,
      SLOT(changeSelection(const QItemSelection &, const QItemSelection &)));

  if(this->HeaderView)
    {
    this->HeaderView->setSelectionModel(this->Selection);
    }

  if(toDelete)
    {
    delete toDelete;
    }

  // Update the view highlights based on the new selection model.
  this->changeSelection(this->Selection->selection(), QItemSelection());
}

void pqFlatTreeView::setSelectionBehavior(
    pqFlatTreeView::SelectionBehavior behavior)
{
  if(this->Behavior != behavior)
    {
    // Clear the current selection before changing the behavior.
    if(this->Selection)
      {
      this->Internal->ShiftStart = QPersistentModelIndex();
      this->Selection->clear();
      }

    this->Behavior = behavior;
    }
}

void pqFlatTreeView::setSelectionMode(pqFlatTreeView::SelectionMode mode)
{
  if(this->Mode != mode)
    {
    // Clear the current selection before changing the mode.
    if(this->Selection)
      {
      this->Internal->ShiftStart = QPersistentModelIndex();
      this->Selection->clear();
      }

    this->Mode = mode;
    }
}

void pqFlatTreeView::setHeader(QHeaderView *headerView)
{
  if(!headerView && this->HeaderView && this->HeaderOwned)
    {
    return;
    }

  // Remove the current header view.
  if(this->HeaderView)
    {
    this->HeaderView->removeEventFilter(this);
    this->disconnect(this->HeaderView, 0, this, 0);
    if(this->HeaderOwned)
      {
      this->HeaderOwned = false;
      delete this->HeaderView;
      }
    else
      {
      this->HeaderView->hide();
      }

    this->HeaderView = 0;
    }

  this->HeaderView = headerView;
  if(this->HeaderView)
    {
    // Make sure the header has the correct parent.
    this->HeaderView->setParent(this->viewport());
    }
  else
    {
    // Set up the default header view.
    this->HeaderView = new QHeaderView(Qt::Horizontal, this->viewport());
    this->HeaderView->setClickable(false);
    this->HeaderView->setSortIndicatorShown(false);
    this->HeaderView->setResizeMode(QHeaderView::Interactive);
    this->HeaderOwned = true;
    }

  this->HeaderView->setModel(this->Model);
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

bool pqFlatTreeView::isIndexHidden(const QModelIndex &index) const
{
  if(!this->Model)
    {
    // If the model is not set, nothing is shown.
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

QModelIndex pqFlatTreeView::getIndexVisibleAt(const QPoint &point) const
{
  if(!this->HeaderView)
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

  // Use the fixed item height to guess the index.
  pqFlatTreeViewItem *item = this->getItemAt(py);
  if(item && item->Cells.size() > 0)
    {
    // Make sure the point is past the pipe connection area.
    if(py < item->ContentsY + pqFlatTreeView::PipeLength)
      {
      return QModelIndex();
      }

    // Make sure the point is within the visible data. If the data
    // width is less than the column width, the point could be in
    // empty space.
    int column = this->HeaderView->logicalIndexAt(point);
    if(column < 0)
      {
      return QModelIndex();
      }

    int cellWidth = this->getWidthSum(item, column);
    if(cellWidth < this->HeaderView->sectionSize(column))
      {
      int columnStart = this->HeaderView->sectionPosition(column);
      if(px > columnStart + cellWidth)
        {
        return QModelIndex();
        }
      }

    return item->Index.sibling(item->Index.row(), column);
    }

  return QModelIndex();
}

QModelIndex pqFlatTreeView::getIndexCellAt(const QPoint &point) const
{
  if(!this->HeaderView)
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

  // Use the fixed item height to guess the index.
  pqFlatTreeViewItem *item = this->getItemAt(py);
  if(item)
    {
    // Make sure the point is past the pipe connection area.
    if(py < item->ContentsY + pqFlatTreeView::PipeLength)
      {
      return QModelIndex();
      }

    int column = this->HeaderView->logicalIndexAt(point);
    if(item && column >= 0)
      {
      return item->Index.sibling(item->Index.row(), column);
      }
    }

  return QModelIndex();
}

void pqFlatTreeView::getSelectionIn(const QRect &rect,
    QItemSelection &items) const
{
  if(!rect.isValid())
    {
    return;
    }

  // Translate the bounding rectangle to contents space.
  QRect bounds = rect;
  bounds.translate(this->horizontalOffset(), this->verticalOffset());
  int top = 0;
  if(!this->HeaderView->isHidden())
    {
    top += this->HeaderView->height();
    }

  // Make sure the bounds intersects the contents area.
  if(!bounds.intersects(QRect(0, top, this->ContentsWidth,
      this->ContentsHeight)))
    {
    return;
    }

  // Find the bounding indexes. Start with the top-left index.
  int start = 0;
  if(bounds.left() >= 0)
    {
    start = this->HeaderView->visualIndexAt(bounds.left());
    }

  pqFlatTreeViewItem *item = 0;
  if(bounds.top() < top)
    {
    item = this->getNextVisibleItem(this->Root);
    }
  else
    {
    item = this->getItemAt(bounds.top());
    }

  if(!item)
    {
    return;
    }

  QModelIndex topLeft = item->Index.sibling(item->Index.row(),
      this->HeaderView->logicalIndex(start));

  // Now find the bottom-right index.
  int end = this->HeaderView->count();
  if(bounds.right() <= this->ContentsWidth)
    {
    end = this->HeaderView->visualIndexAt(bounds.right());
    }

  if(bounds.bottom() > this->ContentsHeight)
    {
    item = this->getLastVisibleItem();
    }
  else
    {
    item = this->getItemAt(bounds.bottom());
    }

  if(!item)
    {
    return;
    }

  QModelIndex bottomRight = item->Index.sibling(item->Index.row(),
      this->HeaderView->logicalIndex(end));
  this->getSelectionIn(topLeft, bottomRight, items);
}

bool pqFlatTreeView::startEditing(const QModelIndex &index)
{
  if(this->Model->flags(index) & Qt::ItemIsEditable)
    {
    // The user might be editing another index.
    this->finishEditing();

    // Get the value for the index.
    QVariant value = this->Model->data(index, Qt::EditRole);
    if(!value.isValid())
      {
      return false;
      }

    // Create an editor appropriate for the value.
    const QItemEditorFactory *factory = QItemEditorFactory::defaultFactory();
    this->Internal->Editor = factory->createEditor(value.type(),
        this->viewport());
    if(!this->Internal->Editor)
      {
      return false;
      }

    this->Internal->Editor->installEventFilter(this);
    this->Internal->Index = index;

    // Set the editor value.
    QByteArray name = factory->valuePropertyName(value.type());
    if(!name.isEmpty())
      {
      this->Internal->Editor->setProperty(name.data(), value);
      }

    QLineEdit *line = qobject_cast<QLineEdit *>(this->Internal->Editor);
    if(line)
      {
      line->selectAll();
      }

    // Update the editor geometry and show the widget.
    this->layoutEditor();
    this->Internal->Editor->show();
    this->Internal->Editor->setFocus();
    this->viewport()->update(QRect(0 - this->horizontalOffset(),
        this->getItem(index)->ContentsY - this->verticalOffset(),
        this->viewport()->width(), this->ItemHeight + 1));
    return true;
    }

  return false;
}

void pqFlatTreeView::finishEditing()
{
  if(this->Internal->Index.isValid() && this->Internal->Editor)
    {
    // Get the new value from the editor.
    QVariant value;
    QModelIndex index = this->Internal->Index;
    const QItemEditorFactory *factory = QItemEditorFactory::defaultFactory();
    QByteArray name = factory->valuePropertyName(value.type());
    if(!name.isEmpty())
      {
      value = this->Internal->Editor->property(name.data());
      }

    // Clean up the editor and repaint the affected area.
    this->cancelEditing();

    // Commit the data to the model. The dataChanged signal handler
    // will update the item layout.
    if(value.isValid())
      {
      this->Model->setData(index, value);
      }
    }
}

void pqFlatTreeView::cancelEditing()
{
  if(this->Internal->Index.isValid() && this->Internal->Editor)
    {
    // Clean up the editor.
    QWidget *editor = this->Internal->Editor;
    this->Internal->Editor = 0;
    delete editor;

    // Repaint the affected area.
    QModelIndex index = this->Internal->Index;
    this->Internal->Index = QPersistentModelIndex();
    this->viewport()->update(QRect(0 - this->horizontalOffset(),
        this->getItem(index)->ContentsY - this->verticalOffset(),
        this->viewport()->width(), this->ItemHeight + 1));
    }
}

void pqFlatTreeView::reset()
{
  // Clean up the editor if it is in use.
  this->cancelEditing();

  // Clean up the current view items.
  this->Internal->ShiftStart = QPersistentModelIndex();
  this->resetRoot();

  // Traverse the model to set up the view items. Only set up the
  // visible items.
  this->addChildItems(this->Root, 1);
  this->layoutItems();
  this->viewport()->update();
}

void pqFlatTreeView::selectAll()
{
  if(this->Mode != pqFlatTreeView::ExtendedSelection)
    {
    return;
    }

  pqFlatTreeViewItem *first = this->getNextVisibleItem(this->Root);
  pqFlatTreeViewItem *last = this->getLastVisibleItem();
  if(!first || !last)
    {
    return;
    }

  QItemSelection items;
  this->getSelectionIn(first->Index, last->Index, items);
  this->Selection->select(items, QItemSelectionModel::ClearAndSelect);
  this->Internal->ShiftStart = first->Index;
  this->Selection->setCurrentIndex(last->Index, QItemSelectionModel::NoUpdate);
  this->scrollTo(last->Index);
}

void pqFlatTreeView::setCurrentIndex(const QModelIndex &index)
{
  if(this->Selection && this->Model &&
      this->Mode != pqFlatTreeView::NoSelection)
    {
    this->Internal->ShiftStart = index;
    if(this->Model->flags(index) & Qt::ItemIsSelectable)
      {
      this->Selection->setCurrentIndex(index,
          QItemSelectionModel::ClearAndSelect);
      }
    else if(this->Mode == pqFlatTreeView::ExtendedSelection)
      {
      this->Selection->setCurrentIndex(index,
          QItemSelectionModel::Clear);
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
    QFontMetrics fm = this->fontMetrics();
    pqFlatTreeViewItem *next = this->getNextVisibleItem(item);
    while(next)
      {
      this->layoutItem(next, point, fm);
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
    QFontMetrics fm = this->fontMetrics();
    pqFlatTreeViewItem *next = this->getNextVisibleItem(item);
    while(next)
      {
      this->layoutItem(next, point, fm);
      next = this->getNextVisibleItem(next);
      }

    // Update the contents size.
    int oldHeight = this->ContentsHeight;
    this->ContentsHeight = point;
    this->updateScrollBars();

    // Remove any selection in the collapsed items.
    if(this->Behavior != pqFlatTreeView::SelectColumns)
      {
      QItemSelection toDeselect;
      pqFlatTreeViewItem *last = this->getNextVisibleItem(item);
      next = this->getNextItem(item);
      while(next && next != last)
        {
        if(this->Behavior == pqFlatTreeView::SelectRows)
          {
          if(next->RowSelected)
            {
            toDeselect.select(next->Index, next->Index);
            }
          }
        else
          {
          QList<pqFlatTreeViewColumn *>::Iterator iter = next->Cells.begin();
          for( ; iter != next->Cells.end(); ++iter)
            {
            if((*iter)->Selected)
              {
              // Put the whole row in the selection.
              int row = next->Index.row();
              toDeselect.select(next->Index.sibling(row, 0),
                  next->Index.sibling(row, next->Cells.size() - 1));
              break;
              }
            }
          }

        next = this->getNextItem(next);
        }

      // If the selection set is not empty, update the selection.
      if(toDeselect.size() > 0)
        {
        if(this->Behavior == pqFlatTreeView::SelectRows)
          {
          this->Selection->select(toDeselect, QItemSelectionModel::Deselect |
              QItemSelectionModel::Rows);
          }
        else
          {
          this->Selection->select(toDeselect, QItemSelectionModel::Deselect);
          }
        }

      // If the current index is no longer visible, set the collapsing
      // index to the current.
      QModelIndex current = this->Selection->currentIndex();
      if(this->isIndexHidden(current))
        {
        this->Selection->setCurrentIndex(item->Index,
            QItemSelectionModel::NoUpdate);
        }

      // If the shift-start index is now hidden, change it to the
      // collapsing index.
      if(this->isIndexHidden(this->Internal->ShiftStart))
        {
        this->Internal->ShiftStart = item->Index;
        }
      }

    // Repaint the area from the item to the end of the view.
    QRect area(0, item->ContentsY, this->ContentsWidth, oldHeight -
        item->ContentsY);
    area.translate(-this->horizontalOffset(), -this->verticalOffset());
    this->viewport()->update(area);
    }
}

void pqFlatTreeView::scrollTo(const QModelIndex &index)
{
  if(!index.isValid() || index.model() != this->Model || !this->HeaderView)
    {
    return;
    }

  pqFlatTreeViewItem *item = this->getItem(index);
  if(item)
    {
    bool atTop = false;
    if(item->ContentsY < this->verticalOffset())
      {
      atTop = true;
      }
    else if(item->ContentsY + this->ItemHeight <= this->verticalOffset() +
        this->viewport()->height())
      {
      return;
      }

    int cy = 0;
    if(atTop)
      {
      if(this->ContentsHeight - item->ContentsY > this->viewport()->height())
        {
        cy = item->ContentsY;
        if(this->HeaderView->isVisible())
          {
          cy -= this->HeaderView->size().height();
          }

        this->verticalScrollBar()->setValue(cy);
        }
      else
        {
        this->verticalScrollBar()->triggerAction(
            QAbstractSlider::SliderToMaximum);
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

void pqFlatTreeView::insertRows(const QModelIndex &parentIndex, int start,
    int end)
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
      index = this->Model->index(start, 0, parentIndex);
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

        QFontMetrics fm = this->fontMetrics();
        pqFlatTreeViewItem *next = this->getNextVisibleItem(item);
        while(next)
          {
          this->layoutItem(next, point, fm);
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

void pqFlatTreeView::startRowRemoval(const QModelIndex &parentIndex, int start,
    int end)
{
  // Get the view item for the parent index. If the view item
  // doesn't exist, it is not visible and no update is necessary.
  pqFlatTreeViewItem *item = this->getItem(parentIndex);
  if(item)
    {
    // TODO: Ensure selection in single selection mode.

    // If one of the indexes is being edited, clean up the editor.
    if(this->Internal->Index.isValid() &&
        this->getItem(this->Internal->Index)->Parent == item &&
        this->Internal->Index.row() >= start &&
        this->Internal->Index.row() <= end)
      {
      this->cancelEditing();
      }

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

void pqFlatTreeView::finishRowRemoval(const QModelIndex &parentIndex,
    int, int)
{
  // Get the view item for the parent index. If the view item
  // doesn't exist, it is not visible and no update is necessary.
  pqFlatTreeViewItem *item = this->getItem(parentIndex);
  if(item)
    {
    // If the root is empty, reset the preferred size list.
    if(this->Root->Items.size() == 0)
      {
      this->resetPreferredSizes();
      }

    // Layout the following items now that the model has finished
    // removing the items.
    int point = item->ContentsY + this->ItemHeight;
    QFontMetrics fm = this->fontMetrics();
    pqFlatTreeViewItem *next = this->getNextVisibleItem(item);
    while(next)
      {
      this->layoutItem(next, point, fm);
      next = this->getNextVisibleItem(next);
      }

    // Update the contents size.
    int oldHeight = this->ContentsHeight;
    this->ContentsHeight = point;
    bool widthChanged = this->updateContentsWidth();
    this->updateScrollBars();

    // If editing an index, update the editor geometry.
    this->layoutEditor();

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

void pqFlatTreeView::insertColumns(const QModelIndex &, int, int)
{
  // TODO
}

void pqFlatTreeView::startColumnRemoval(const QModelIndex &, int, int)
{
  // TODO
}

void pqFlatTreeView::finishColumnRemoval(const QModelIndex &, int, int)
{
  // TODO
}

void pqFlatTreeView::updateData(const QModelIndex &topLeft,
    const QModelIndex &bottomRight)
{
  // The changed indexes must have the same parent.
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
    QFontMetrics fm = this->fontMetrics();
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
          this->layoutItem(item, point, fm);
          }
        }
      }

    // If the items are visible, repaint the affected area.
    if(itemsVisible)
      {
      bool widthChanged = this->updateContentsWidth();
      this->updateScrollBars();

      // If the index being edited has changed, update the editor value.
      if(this->Internal->Index.isValid() &&
          this->getItem(this->Internal->Index)->Parent == parentItem &&
          this->Internal->Index.row() >= topLeft.row() &&
          this->Internal->Index.row() <= bottomRight.row() &&
          this->Internal->Index.column() >= topLeft.column())
        {
        // Update the editor geometry.
        this->layoutEditor();
        if(this->Internal->Index.column() >= bottomRight.column())
          {
          QVariant value = this->Model->data(this->Internal->Index,
              Qt::EditRole);
          const QItemEditorFactory *factory =
              QItemEditorFactory::defaultFactory();
          QByteArray name = factory->valuePropertyName(value.type());
          if(!name.isEmpty())
            {
            this->Internal->Editor->setProperty(name.data(), value);
            }
          }
        }

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

void pqFlatTreeView::keyPressEvent(QKeyEvent *e)
{
  if(!this->Model)
    {
    e->ignore();
    return;
    }

  int column = 0;
  QModelIndex index;
  bool handled = true;
  pqFlatTreeViewItem *item = 0;
  QModelIndex current = this->Selection->currentIndex();
  switch(e->key())
    {
    // Navigation/selection keys ------------------------------------
    case Qt::Key_Down:
      {
      // If the control key is down, don't change the selection.
      if(e->modifiers() & Qt::ControlModifier ||
          this->Mode == pqFlatTreeView::NoSelection)
        {
        this->verticalScrollBar()->triggerAction(
            QAbstractSlider::SliderSingleStepAdd);
        }
      else if(this->Behavior != pqFlatTreeView::SelectColumns)
        {
        item = this->Root;
        if(current.isValid())
          {
          item = this->getItem(current);
          column = current.column();
          }

        item = this->getNextVisibleItem(item);
        if(item)
          {
          index = item->Index.sibling(item->Index.row(), column);
          if(e->modifiers() & Qt::ShiftModifier &&
              this->Mode == pqFlatTreeView::ExtendedSelection &&
              this->Internal->ShiftStart.isValid())
            {
            QItemSelection items;
            this->getSelectionIn(this->Internal->ShiftStart, index, items);
            this->Selection->select(items,
                QItemSelectionModel::ClearAndSelect);
            this->Selection->setCurrentIndex(index,
                QItemSelectionModel::NoUpdate);
            }
          else
            {
            this->setCurrentIndex(index);
            }

          this->scrollTo(index);
          }
        }

      break;
      }
    case Qt::Key_Up:
      {
      if(e->modifiers() & Qt::ControlModifier ||
          this->Mode == pqFlatTreeView::NoSelection)
        {
        this->verticalScrollBar()->triggerAction(
            QAbstractSlider::SliderSingleStepSub);
        }
      else if(this->Behavior != pqFlatTreeView::SelectColumns)
        {
        if(current.isValid())
          {
          item = this->getPreviousVisibleItem(this->getItem(current));
          column = current.column();
          }
        else
          {
          item = this->getNextVisibleItem(this->Root);
          }

        if(item)
          {
          index = item->Index.sibling(item->Index.row(), column);
          if(e->modifiers() & Qt::ShiftModifier &&
              this->Mode == pqFlatTreeView::ExtendedSelection &&
              this->Internal->ShiftStart.isValid())
            {
            QItemSelection items;
            this->getSelectionIn(this->Internal->ShiftStart, index, items);
            this->Selection->select(items,
                QItemSelectionModel::ClearAndSelect);
            this->Selection->setCurrentIndex(index,
                QItemSelectionModel::NoUpdate);
            }
          else
            {
            this->setCurrentIndex(index);
            }

          this->scrollTo(index);
          }
        }

      break;
      }
    case Qt::Key_Left:
      {
      if(e->modifiers() & Qt::ControlModifier ||
          this->Mode == pqFlatTreeView::NoSelection)
        {
        this->horizontalScrollBar()->triggerAction(
            QAbstractSlider::SliderSingleStepSub);
        }
      else if(this->Behavior == pqFlatTreeView::SelectColumns)
        {
        // TODO
        }
      else if(current.isValid())
        {
        item = this->getItem(current);
        if(current.column() == 0 ||
            this->Behavior == pqFlatTreeView::SelectRows)
          {
          if(item->Expandable && item->Expanded)
            {
            this->collapse(current);
            }
          else
            {
            // Find an expandable ancestor.
            pqFlatTreeViewItem *parentItem = item->Parent;
            while(parentItem)
              {
              if(parentItem->Expandable)
                {
                break;
                }
              else if(parentItem->Items.size() > 1)
                {
                break;
                }

              parentItem = parentItem->Parent;
              }

            if(parentItem && parentItem != this->Root)
              {
              this->setCurrentIndex(parentItem->Index);
              this->scrollTo(parentItem->Index);
              }
            }
          }
        else
          {
          // TODO: Move the selection to the left one column.
          }
        }

      break;
      }
    case Qt::Key_Right:
      {
      if(e->modifiers() & Qt::ControlModifier ||
          this->Mode == pqFlatTreeView::NoSelection)
        {
        this->horizontalScrollBar()->triggerAction(
            QAbstractSlider::SliderSingleStepAdd);
        }
      else if(this->Behavior == pqFlatTreeView::SelectColumns)
        {
        // TODO
        }
      else if(current.isValid())
        {
        item = this->getItem(current);
        if(current.column() == 0 ||
            this->Behavior == pqFlatTreeView::SelectRows)
          {
          if(item->Expandable && !item->Expanded)
            {
            this->expand(current);
            }
          else if(item->Expandable || item->Items.size() > 1)
            {
            // Select the first child.
            index = item->Items[0]->Index;
            this->setCurrentIndex(index);
            this->scrollTo(index);
            }
          }
        else
          {
          // TODO: Move the selection to the right one column.
          }
        }

      break;
      }
    case Qt::Key_PageUp:
      {
      if(e->modifiers() & Qt::ControlModifier ||
          this->Mode == pqFlatTreeView::NoSelection)
        {
        this->verticalScrollBar()->triggerAction(
            QAbstractSlider::SliderPageStepSub);
        }
      else if(this->Behavior != pqFlatTreeView::SelectColumns)
        {
        // Find the index at the top of the current page.
        int py = this->verticalOffset() + pqFlatTreeView::PipeLength;
        item = this->getItemAt(py);
        if(!item && this->HeaderView->isVisible())
          {
          item = this->getNextVisibleItem(this->Root);
          }

        pqFlatTreeViewItem *currentItem = 0;
        if(current.isValid())
          {
          currentItem = this->getItem(current);
          column = current.column();
          }

        // If the current index is at the top of the page, get the
        // item on the next page up.
        if(currentItem && currentItem == item)
          {
          item = this->getItemAt(py - this->verticalScrollBar()->pageStep());
          if(!item)
            {
            item = this->getNextVisibleItem(this->Root);
            }
          }

        if(item && item != currentItem)
          {
          index = item->Index.sibling(item->Index.row(), column);
          if(e->modifiers() & Qt::ShiftModifier &&
              this->Mode == pqFlatTreeView::ExtendedSelection &&
              this->Internal->ShiftStart.isValid())
            {
            QItemSelection items;
            this->getSelectionIn(this->Internal->ShiftStart, index, items);
            this->Selection->select(items,
                QItemSelectionModel::ClearAndSelect);
            this->Selection->setCurrentIndex(index,
                QItemSelectionModel::NoUpdate);
            }
          else
            {
            this->setCurrentIndex(index);
            }

          this->scrollTo(index);
          }
        }

      break;
      }
    case Qt::Key_PageDown:
      {
      if(e->modifiers() & Qt::ControlModifier ||
          this->Mode == pqFlatTreeView::NoSelection)
        {
        this->verticalScrollBar()->triggerAction(
            QAbstractSlider::SliderPageStepAdd);
        }
      else if(this->Behavior != pqFlatTreeView::SelectColumns)
        {
        // Find the index at the bottom of the current page.
        int py = this->verticalOffset() - (this->ItemHeight / 2) +
            this->verticalScrollBar()->pageStep();
        item = this->getItemAt(py);
        if(!item)
          {
          item = this->getLastVisibleItem();
          }

        pqFlatTreeViewItem *currentItem = 0;
        if(current.isValid())
          {
          currentItem = this->getItem(current);
          column = current.column();
          }

        // If the current index is at the bottom of the page, get the
        // item on the next page down.
        if(currentItem && currentItem == item)
          {
          item = this->getItemAt(py + this->verticalScrollBar()->pageStep());
          if(!item)
            {
            item = this->getLastVisibleItem();
            }
          }

        if(item && item != currentItem)
          {
          index = item->Index.sibling(item->Index.row(), column);
          if(e->modifiers() & Qt::ShiftModifier &&
              this->Mode == pqFlatTreeView::ExtendedSelection &&
              this->Internal->ShiftStart.isValid())
            {
            QItemSelection items;
            this->getSelectionIn(this->Internal->ShiftStart, index, items);
            this->Selection->select(items,
                QItemSelectionModel::ClearAndSelect);
            this->Selection->setCurrentIndex(index,
                QItemSelectionModel::NoUpdate);
            }
          else
            {
            this->setCurrentIndex(index);
            }

          this->scrollTo(index);
          }
        }

      break;
      }
    case Qt::Key_Home:
      {
      if(e->modifiers() & Qt::ControlModifier ||
          this->Mode == pqFlatTreeView::NoSelection)
        {
        this->verticalScrollBar()->triggerAction(
            QAbstractSlider::SliderToMinimum);
        }
      else if(this->Behavior == pqFlatTreeView::SelectColumns)
        {
        // TODO
        }
      else
        {
        item = this->getNextVisibleItem(this->Root);
        if(item)
          {
          if(e->modifiers() & Qt::ShiftModifier &&
              this->Mode == pqFlatTreeView::ExtendedSelection &&
              this->Internal->ShiftStart.isValid())
            {
            QItemSelection items;
            this->getSelectionIn(this->Internal->ShiftStart, item->Index,
                items);
            this->Selection->select(items,
                QItemSelectionModel::ClearAndSelect);
            this->Selection->setCurrentIndex(item->Index,
                QItemSelectionModel::NoUpdate);
            }
          else
            {
            this->setCurrentIndex(item->Index);
            }

          this->scrollTo(item->Index);
          }
        }

      break;
      }
    case Qt::Key_End:
      {
      if(e->modifiers() & Qt::ControlModifier ||
          this->Mode == pqFlatTreeView::NoSelection)
        {
        this->verticalScrollBar()->triggerAction(
            QAbstractSlider::SliderToMaximum);
        }
      else if(this->Behavior == pqFlatTreeView::SelectColumns)
        {
        // TODO
        }
      else
        {
        item = this->getLastVisibleItem();
        if(item)
          {
          if(e->modifiers() & Qt::ShiftModifier &&
              this->Mode == pqFlatTreeView::ExtendedSelection &&
              this->Internal->ShiftStart.isValid())
            {
            QItemSelection items;
            this->getSelectionIn(this->Internal->ShiftStart, item->Index,
                items);
            this->Selection->select(items,
                QItemSelectionModel::ClearAndSelect);
            this->Selection->setCurrentIndex(item->Index,
                QItemSelectionModel::NoUpdate);
            }
          else
            {
            this->setCurrentIndex(item->Index);
            }

          this->scrollTo(item->Index);
          }
        }

      break;
      }

    // Selection keys -----------------------------------------------
    case Qt::Key_Space:
      {
      if(current.isValid() &&
          this->Mode == pqFlatTreeView::ExtendedSelection &&
          (this->Model->flags(current) & Qt::ItemIsSelectable))
        {
        this->Internal->ShiftStart = current;
        if(e->modifiers() & Qt::ControlModifier)
          {
          this->Selection->select(current, QItemSelectionModel::Toggle);
          }
        else
          {
          this->Selection->select(current, QItemSelectionModel::Select);
          }
        }

      handled = false;
      break;
      }
    case Qt::Key_A:
      {
      if(e->modifiers() & Qt::ControlModifier)
        {
        this->selectAll();
        }
      else
        {
        handled = false;
        }

      break;
      }

    // OS specific keys ---------------------------------------------
#ifdef Q_WS_MAC
    case Qt::Key_Enter:
    case Qt::Key_Return:
      {
      if(!startEditing(current))
        {
        e->ignore();
        return;
        }

      break;
      }
    case Qt::Key_O:
      {
      if(e->modifiers() & Qt::ControlModifier)
        {
        if(current.isValid())
          {
          emit this->activated(current);
          }
        }
      else
        {
        handled = false;
        }
      break;
      }
#else
    case Qt::Key_F2:
      {
      if(!startEditing(current))
        {
        e->ignore();
        return;
        }

      break;
      }
    case Qt::Key_Enter:
    case Qt::Key_Return:
    case Qt::Key_Select:
      {
      if(current.isValid())
        {
        emit this->activated(current);
        }

      break;
      }
#endif

    default:
      {
      handled = false;
      }
    }

  // If the key is not handled, it may be a keyboard search.
  if(!handled && !(e->modifiers() &
      (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) &&
      this->Mode != pqFlatTreeView::NoSelection)
    {
    QString text = e->text();
    if(!text.isEmpty())
      {
      handled = true;
      this->keyboardSearch(text);
      }
    }

  if(handled)
    {
    e->accept();
    }
  else
    {
    e->ignore();
    }
}

void pqFlatTreeView::keyboardSearch(const QString &)
{
  // TODO
}

void pqFlatTreeView::mousePressEvent(QMouseEvent *e)
{
  if(!this->HeaderView || !this->Model || e->button() == Qt::MidButton)
    {
    e->ignore();
    return;
    }

  // If an index is being edited, a click in the viewport should end
  // the editing.
  this->finishEditing();

  e->accept();
  QModelIndex index;
  if(this->Mode == pqFlatTreeView::SingleSelection)
    {
    index = this->getIndexCellAt(e->pos());
    }
  else
    {
    index = this->getIndexVisibleAt(e->pos());
    }

  // First, check for a click on the expand/collapse control.
  pqFlatTreeViewItem *item = this->getItem(index);
  int px = e->pos().x() + this->horizontalOffset();
  if(index.isValid() && item && index.column() == 0)
    {
    int itemStart = this->HeaderView->sectionPosition(index.column()) +
        item->Indent;
    if(item->Expandable && e->button() == Qt::LeftButton)
      {
      if(px < itemStart - this->IndentWidth)
        {
        if(this->Mode == pqFlatTreeView::ExtendedSelection)
          {
          index = QModelIndex();
          }
        }
      else if(px < itemStart)
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
    else if(px < itemStart && this->Mode == pqFlatTreeView::ExtendedSelection)
      {
      index = QModelIndex();
      }
    }

  bool itemEnabled = true;
  if(index.isValid() && !(this->Model->flags(index) & Qt::ItemIsEnabled))
    {
    itemEnabled = false;
    index = QModelIndex();
    }

  // Next, check for a change in the selection.
  bool itemSelected = this->Selection->isSelected(index);
  if(this->Mode != pqFlatTreeView::NoSelection &&
    !(e->button() == Qt::RightButton && itemSelected))
    {
    if(this->Mode == pqFlatTreeView::SingleSelection)
      {
      // Select the index and make it the current index. Do nothing
      // if it is already selected or is not valid.
      if(index.isValid() && !itemSelected &&
          (this->Model->flags(index) & Qt::ItemIsSelectable))
        {
        this->Selection->setCurrentIndex(index,
            QItemSelectionModel::ClearAndSelect);
        }
      }
    else if(e->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier))
      {
      // Only change the selection if the index is valid and selectable.
      if(index.isValid() && (this->Model->flags(index) & Qt::ItemIsSelectable))
        {
        if(e->modifiers() & Qt::ControlModifier)
          {
          // Toggle the selection of the index.
          this->Selection->setCurrentIndex(index, QItemSelectionModel::Toggle);
          }
        else if(this->Internal->ShiftStart.isValid())
          {
          QItemSelection items;
          this->getSelectionIn(this->Internal->ShiftStart, index, items);
          this->Selection->select(items, QItemSelectionModel::ClearAndSelect);
          this->Selection->setCurrentIndex(index,
              QItemSelectionModel::NoUpdate);
          }
        else
          {
          this->Selection->setCurrentIndex(index,
              QItemSelectionModel::ClearAndSelect);
          }
        }
      }
    else
      {
      // If the index is disabled or not selectable, don't change the
      // selection. Otherwise, clear the selection, and set the new
      // index as selected and the current.
      if(itemEnabled && (!index.isValid() ||
          (this->Model->flags(index) & Qt::ItemIsSelectable)))
        {
        if(index.isValid())
          {
          this->Internal->ShiftStart = index;
          this->Selection->setCurrentIndex(index,
              QItemSelectionModel::ClearAndSelect);
          }
        else
          {
          // If there is a current index, keep it current.
          index = this->Selection->currentIndex();
          if(index.isValid())
            {
            this->Selection->setCurrentIndex(index,
                QItemSelectionModel::Clear);
            }
          else
            {
            this->Selection->clear();
            }
          }
        }
      }
    }

  if(index.isValid() && e->button() == Qt::LeftButton)
    {
    // Finally, check for an edit triggering click. The click should be
    // over the display portion of the item.
    // Note: The only supported trigger is selected-clicked.
    bool sendClicked = true;
    if(item && itemSelected && item->Cells.size() > 0 &&
        !(e->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier)))
      {
      int itemWidth = this->getWidthSum(item, index.column());
      int columnStart = this->HeaderView->sectionPosition(index.column());
      if(px < columnStart + itemWidth)
        {
        columnStart += itemWidth - item->Cells[index.column()]->Width +
            pqFlatTreeView::TextMargin;
        if(px >= columnStart)
          {
          sendClicked = !this->startEditing(index);
          }
        }
      }

    // If the item was not edited, send the clicked signal.
    if(sendClicked)
      {
      emit this->clicked(index);
      }
    }
}

void pqFlatTreeView::mouseMoveEvent(QMouseEvent *e)
{
  // TODO
  e->ignore();
}

void pqFlatTreeView::mouseReleaseEvent(QMouseEvent *e)
{
  // TODO
  e->ignore();
}

void pqFlatTreeView::mouseDoubleClickEvent(QMouseEvent *e)
{
  if(!this->HeaderView || e->button() != Qt::LeftButton)
    {
    e->ignore();
    return;
    }

  e->accept();
  QModelIndex index;
  if(this->Mode == pqFlatTreeView::SingleSelection)
    {
    index = this->getIndexCellAt(e->pos());
    }
  else
    {
    index = this->getIndexVisibleAt(e->pos());
    }

  pqFlatTreeViewItem *item = this->getItem(index);
  if(index.isValid() && item && item->Cells.size() > 0)
    {
    if(index.column() == 0)
      {
      int itemStart = this->HeaderView->sectionPosition(index.column()) +
          item->Indent;
      int px = e->pos().x() + this->horizontalOffset();
      if(item->Expandable)
        {
        if(px >= itemStart - this->IndentWidth ||
            this->Mode == pqFlatTreeView::SingleSelection)
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
        else if(this->Mode == pqFlatTreeView::ExtendedSelection)
          {
          return;
          }
        }
      else if(px < itemStart && this->Mode == pqFlatTreeView::ExtendedSelection)
        {
        return;
        }
      }

    if(this->Model->flags(index) & Qt::ItemIsEnabled)
      {
      emit this->activated(index);
      }
    }
}

void pqFlatTreeView::wheelEvent(QWheelEvent *e)
{
  if(this->verticalScrollBar()->isVisible())
    {
    QApplication::sendEvent(this->verticalScrollBar(), e);
    }
  else
    {
    e->ignore();
    }
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

  QAbstractScrollArea::resizeEvent(e);
}

bool pqFlatTreeView::viewportEvent(QEvent *e)
{
  if(e->type() == QEvent::FontChange)
    {
    // Layout all the items for the new font.
    this->FontChanged = true;
    this->layoutItems();
    this->layoutEditor();
    this->viewport()->update();
    }

  return QAbstractScrollArea::viewportEvent(e);
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
  QStyleOptionViewItem options = this->getViewOptions();
  int fontHeight = options.fontMetrics.height();
  int fontAscent = options.fontMetrics.ascent();
  painter.setFont(options.font);
  QColor branchColor(Qt::darkGray);
  QColor expandColor(Qt::black);

  // Draw the column selection first so it is behind the text.
  int columnWidth = 0;
  int totalHeight = this->viewport()->height();
  if(this->ContentsHeight > totalHeight)
    {
    totalHeight = this->ContentsHeight;
    }

  for(i = 0; i < this->Root->Cells.size(); i++)
    {
    if(this->Root->Cells[i]->Selected)
      {
      px = this->HeaderView->sectionPosition(i);
      columnWidth = this->HeaderView->sectionSize(i);
      painter.fillRect(px, 0, columnWidth, totalHeight,
          options.palette.highlight());
      }
    }

  // Loop through the visible items. Paint the items that are in the
  // repaint area.
  QModelIndex index;
  int columns = this->Model->columnCount(this->Root->Index);
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
          py = item->ContentsY + pqFlatTreeView::PipeLength;
          columnWidth = this->HeaderView->sectionSize(i) -
              pqFlatTreeView::TextMargin;

          // If the item is selected, draw a highlight in the background.
          QRect highlightRect;
          index = item->Index.sibling(item->Index.row(), i);
          if(this->Behavior == pqFlatTreeView::SelectItems &&
              (item->Cells[i]->Selected ||
              index == this->Selection->currentIndex()))
            {
            int ox = 0;
            itemWidth = this->getWidthSum(item, i) -
                pqFlatTreeView::TextMargin;
            if(!options.showDecorationSelected)
              {
              // If the decoration is not supposed to be selected,
              // adjust the position and width of the highlight.
              ox = itemWidth - item->Cells[i]->Width;
              }

            if(itemWidth > columnWidth)
              {
              itemWidth = columnWidth;
              }

            // Add a little length to the highlight rectangle to see
            // the text better.
            highlightRect.setRect(px + ox, py, itemWidth - ox + 2, trueHeight);
            if(item->Cells[i]->Selected)
              {
              painter.fillRect(highlightRect, options.palette.highlight());
              }
            }

          // Draw the branches for column 0. Adjust the position and
          // width for the indent.
          if(i == 0)
            {
            this->drawBranches(painter, item, halfIndent, branchColor,
                expandColor, options);
            px += item->Indent;
            columnWidth -= item->Indent;
            }

          // If the index has decoration, draw that next. Get the icon
          // for this item from the model. QVariant won't automatically
          // convert from pixmap to icon, so it has to be done here.
          QIcon icon;
          QPixmap pixmap;
          QVariant decoration = this->Model->data(index, Qt::DecorationRole);
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

            // TODO: Clip the icon to column width.
            pixmap = icon.pixmap(options.decorationSize);
            painter.drawPixmap(px + 1, py + 1, pixmap);

            // Add extra padding to the x coordinate to put space
            // between the icon and the text.
            itemWidth = this->IndentWidth + pqFlatTreeView::TextMargin;
            px += itemWidth;
            columnWidth -= itemWidth;
            py = item->ContentsY + pqFlatTreeView::PipeLength;
            }

          // Draw in the display data.
          QVariant indexData = this->Model->data(index);
          if(indexData.type() == QVariant::Pixmap ||
              indexData.canConvert(QVariant::Icon))
            {
            if(indexData.type() == QVariant::Pixmap)
              {
              pixmap = qvariant_cast<QPixmap>(indexData);
              if(pixmap.height() > trueHeight)
                {
                pixmap = pixmap.scaledToHeight(trueHeight);
                }
              }
            else
              {
              icon = qvariant_cast<QIcon>(indexData);
              pixmap = icon.pixmap(options.decorationSize);
              }

            if(!pixmap.isNull() && columnWidth > 0)
              {
              // Adjust the vertical alignment according to the style.
              if(options.displayAlignment & Qt::AlignVCenter)
                {
                py += (trueHeight - pixmap.height()) / 2;
                }
              else if(options.displayAlignment & Qt::AlignBottom)
                {
                py += trueHeight - pixmap.height();
                }

              // TODO: Clip the pixmap to column width.
              painter.drawPixmap(px, py, pixmap);
              }
            }
          else
            {
            QString text = indexData.toString();
            if(!text.isEmpty() && columnWidth > 0)
              {
              // Set the text color based on the highlighted state.
              if(item->RowSelected || this->Root->Cells[i]->Selected ||
                  item->Cells[i]->Selected)
                {
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
                text = QAbstractItemDelegate::elidedText(options.fontMetrics,
                    columnWidth, options.textElideMode, text);
                }

              // TODO: Clip text drawing to column width.
              painter.drawText(px, py + fontAscent, text);
              }
            }

          // If this is the current index, draw the focus rectangle.
          if(this->Behavior == pqFlatTreeView::SelectItems &&
              index == this->Selection->currentIndex())
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

            opt.rect = highlightRect;
            QApplication::style()->drawPrimitive(QStyle::PE_FrameFocusRect,
                &opt, &painter);
            }
          }

        // If this is the current row, draw the current outline.
        if(this->Behavior == pqFlatTreeView::SelectRows)
          {
          index = this->Selection->currentIndex();
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
  if(this->Behavior == pqFlatTreeView::SelectColumns)
  {
    index = this->Selection->currentIndex();
    if(index.isValid())
      {
      }
  }

  // If the user is editing an index, draw a box around the editor.
  if(this->Internal->Editor)
    {
    painter.setPen(QColor(Qt::black));
    QRect editorRect = this->Internal->Editor->geometry();
    editorRect.translate(this->horizontalOffset(), this->verticalOffset());
    editorRect.setTop(editorRect.top() - 1);
    editorRect.setLeft(editorRect.left() - 1);
    painter.drawRect(editorRect);
    }
}

QStyleOptionViewItem pqFlatTreeView::getViewOptions() const
{
  QStyleOptionViewItem option;
  option.init(this);
  option.font = this->font();
  option.state &= ~QStyle::State_HasFocus;
  //if(d->iconSize.isValid())
  //  {
  //  option.decorationSize = d->iconSize;
  //  }
  //else
    {
    int pm = style()->pixelMetric(QStyle::PM_SmallIconSize);
    option.decorationSize = QSize(pm, pm);
    }

  option.decorationPosition = QStyleOptionViewItem::Left;
  option.decorationAlignment = Qt::AlignCenter;
  option.displayAlignment = QStyle::visualAlignment(this->layoutDirection(),
      Qt::AlignLeft | Qt::AlignVCenter);
  //option.textElideMode = d->textElideMode;
  option.rect = QRect();
  option.showDecorationSelected = style()->styleHint(
      QStyle::SH_ItemView_ShowDecorationSelected);
  return option;
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
  if(this->Behavior == pqFlatTreeView::SelectItems)
    {
    // If the last index is valid, add its row to the repaint region.
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

    // If the new index is valid, add its row to the repaint region.
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
  if(this->Behavior == pqFlatTreeView::SelectRows)
    {
    // If the last index is valid, add its row to the repaint region.
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

    // If the new index is valid, add its row to the repaint region.
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
  if(this->Behavior == pqFlatTreeView::SelectColumns)
    {
    this->viewport()->update();
    }
}

void pqFlatTreeView::changeSelection(const QItemSelection &selected,
    const QItemSelection &deselected)
{
  if(!this->HeaderView)
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
      if(this->Behavior == pqFlatTreeView::SelectColumns)
        {
        end = (*iter).right();
        for(start = (*iter).left(); start <= end; start++)
          {
          this->Root->Cells[start]->Selected = false;
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

          if(this->Behavior == pqFlatTreeView::SelectRows)
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
      if(this->Behavior == pqFlatTreeView::SelectColumns)
        {
        end = (*iter).right();
        for(start = (*iter).left(); start <= end; start++)
          {
          this->Root->Cells[start]->Selected = true;
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

          if(this->Behavior == pqFlatTreeView::SelectRows)
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

  if(this->Behavior == pqFlatTreeView::SelectColumns &&
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

void pqFlatTreeView::resetRoot()
{
  // Clean up the child items.
  QList<pqFlatTreeViewItem *>::Iterator iter = this->Root->Items.begin();
  for( ; iter != this->Root->Items.end(); ++iter)
    {
    delete *iter;
    }

  this->Root->Items.clear();

  // Clean up the cells.
  QList<pqFlatTreeViewColumn *>::Iterator jter = this->Root->Cells.begin();
  for( ; jter != this->Root->Cells.end(); ++jter)
    {
    delete *jter;
    }

  this->Root->Cells.clear();
  if(this->Root->Index.isValid())
    {
    this->Root->Index = QPersistentModelIndex();
    }
}

void pqFlatTreeView::resetPreferredSizes()
{
  QList<pqFlatTreeViewColumn *>::Iterator iter = this->Root->Cells.begin();
  for( ; iter != this->Root->Cells.end(); ++iter)
    {
    (*iter)->Width = 0;
    }
}

void pqFlatTreeView::layoutEditor()
{
  if(this->Internal->Index.isValid() && this->Internal->Editor)
    {
    int column = this->Internal->Index.column();
    pqFlatTreeViewItem *item = this->getItem(this->Internal->Index);
    int ex = this->HeaderView->sectionPosition(column);
    int columnWidth = this->HeaderView->sectionSize(column);
    int itemWidth = this->getWidthSum(item, column);
    int editWidth = itemWidth;
    if(editWidth < columnWidth)
      {
      // Add some extra space to the editor.
      editWidth += pqFlatTreeView::DoubleTextMargin;
      if(editWidth > columnWidth)
        {
        editWidth = columnWidth;
        }
      }

    // Figure out how much space is taken up by decoration.
    int indent = itemWidth - item->Cells[column]->Width -
        pqFlatTreeView::TextMargin;
    if(indent > 0)
      {
      ex += indent;
      editWidth -= indent;
      }

    int ey = item->ContentsY + pqFlatTreeView::PipeLength;
    int editHeight = this->ItemHeight - pqFlatTreeView::PipeLength;

    // Adjust the location to viewport coordinates and set the size.
    ex -= this->horizontalOffset();
    ey -= this->verticalOffset();
    this->Internal->Editor->setGeometry(ex, ey, editWidth, editHeight);
    }
}

void pqFlatTreeView::layoutItems()
{
  if(this->HeaderView)
    {
    // Determine the item height based on the font and icon size.
    // The minimum indent width should be 18 to fit the +/- icon.
    QStyleOptionViewItem options = this->getViewOptions();
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
      point = this->HeaderView->height();
      }

    // Make sure the preferred size array is allocated.
    int columnCount = this->Model->columnCount(this->Root->Index) -
        this->Root->Cells.size();
    for(int i=0; i < columnCount; i++)
      {
      this->Root->Cells.append(new pqFlatTreeViewColumn());
      }

    // Reset the preferred column sizes.
    this->resetPreferredSizes();

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
    if(item->Cells.size() == 0)
      {
      for(i = 0; i < this->Root->Cells.size(); i++)
        {
        item->Cells.append(new pqFlatTreeViewColumn());
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
      preferredWidth = this->getWidthSum(item, 0);
      if(preferredWidth > this->Root->Cells[0]->Width)
        {
        this->Root->Cells[0]->Width = preferredWidth;
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
        }

      preferredWidth = this->getWidthSum(item, i);
      if(preferredWidth > this->Root->Cells[i]->Width)
        {
        this->Root->Cells[i]->Width = preferredWidth;
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
    return this->getViewOptions().decorationSize.width();
    }
  else
    {
    // Find the font width for the string.
    return fm.width(indexData.toString());
    }
}

int pqFlatTreeView::getWidthSum(pqFlatTreeViewItem *item, int column) const
{
  int total = item->Cells[column]->Width + pqFlatTreeView::TextMargin;
  QModelIndex index = item->Index;
  if(column == 0)
    {
    total += item->Indent;
    }
  else
    {
    index = index.sibling(index.row(), column);
    }

  QVariant icon = index.data(Qt::DecorationRole);
  if(icon.isValid())
    {
    total += this->IndentWidth + pqFlatTreeView::TextMargin;
    }

  return total;
}

bool pqFlatTreeView::updateContentsWidth()
{
  bool sectionSizeChanged = false;
  int oldContentsWidth = this->ContentsWidth;
  this->ContentsWidth = 0;
  if(this->HeaderView)
    {
    // Manage the header section sizes if the header is not visible.
    if(this->ManageSizes || this->HeaderView->isHidden())
      {
      this->InUpdateWidth = true;
      int newWidth = 0;
      int oldWidth = 0;
      for(int i = 0; i < this->Root->Cells.size(); i++)
        {
        oldWidth = this->HeaderView->sectionSize(i);
        newWidth = this->HeaderView->sectionSizeHint(i);
        if(newWidth < this->Root->Cells[i]->Width)
          {
          newWidth = this->Root->Cells[i]->Width;
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
    if(this->Model->canFetchMore(item->Index))
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
        this->Model->fetchMore(item->Index);
        }
      }

    int count = this->Model->rowCount(item->Index);
    item->Expandable = parentChildCount > 1 && count > 0;
    if(item->Expanded || !item->Expandable)
      {
      // Set up the parent and model index for each added child.
      // The model's hierarchical data should be in column 0.
      QModelIndex index;
      pqFlatTreeViewItem *child = 0;
      for(int i = 0; i < count; i++)
        {
        index = this->Model->index(i, 0, item->Index);
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
  if(index.isValid() && index.model() != this->Model)
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

pqFlatTreeViewItem *pqFlatTreeView::getItemAt(int contentsY) const
{
  if(contentsY > this->ContentsHeight)
    {
    return 0;
    }

  // Use the fixed item height to guess the index.
  int index = contentsY;
  if(this->HeaderView->isVisible())
    {
    index -= this->HeaderView->size().height();
    if(index < 0)
      {
      return 0;
      }
    }

  index = index / this->ItemHeight;
  pqFlatTreeViewItem *item = this->getNextVisibleItem(this->Root);
  for(int i = 0; i < index && item; i++)
    {
    item = this->getNextVisibleItem(item);
    }

  return item;
}

pqFlatTreeViewItem *pqFlatTreeView::getNextItem(pqFlatTreeViewItem *item) const
{
  if(item)
    {
    if(item->Items.size() > 0)
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
      while(item->Items.size() > 0 && (!item->Expandable || item->Expanded))
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
    while(item->Items.size() > 0 && (!item->Expandable || item->Expanded))
      {
      item = item->Items.last();
      }

    return item;
    }

  return 0;
}

void pqFlatTreeView::getSelectionIn(const QModelIndex &topLeft,
    const QModelIndex &bottomRight, QItemSelection &items) const
{
  pqFlatTreeViewItem *item = this->getItem(topLeft);
  pqFlatTreeViewItem *last = this->getItem(bottomRight);
  if(!item || !last)
    {
    return;
    }

  if(last->ContentsY < item->ContentsY)
    {
    pqFlatTreeViewItem *temp = item;
    item = last;
    last = temp;
    }

  // Find the list of columns in the bounding area. Use the visual
  // column indexes for the bounds.
  QList<int> columns;
  int start = this->HeaderView->visualIndex(topLeft.column());
  int end = this->HeaderView->visualIndex(bottomRight.column());
  for(int i = start; i <= end; i++)
    {
    columns.append(this->HeaderView->logicalIndex(i));
    }

  QModelIndex index;
  QList<int>::Iterator iter;
  last = this->getNextVisibleItem(last);
  while(item && item != last)
    {
    for(iter = columns.begin(); iter != columns.end(); ++iter)
      {
      index = item->Index.sibling(item->Index.row(), *iter);
      if(this->Model->flags(index) & Qt::ItemIsSelectable)
        {
        items.select(index, index);
        }
      }

    item = this->getNextVisibleItem(item);
    }
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


