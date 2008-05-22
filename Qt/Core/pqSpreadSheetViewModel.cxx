/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqSpreadSheetViewModel.cxx,v $

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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

========================================================================*/
#include "pqSpreadSheetViewModel.h"

// Server Manager Includes.
#include "vtkEventQtSlotConnect.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkIndexBasedBlockFilter.h"
#include "vtkInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkSelection.h"
#include "vtkSmartPointer.h"
#include "vtkSMInputProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMSpreadSheetRepresentationProxy.h"
#include "vtkStdString.h"
#include "vtkTable.h"
#include "vtkUnsignedIntArray.h"
#include "vtkVariant.h"

// Qt Includes.
#include <QTimer>
#include <QItemSelectionModel>
#include <QtDebug>
#include <QPointer>

// ParaView Includes.
#include "pqSMAdaptor.h"
#include "pqDataRepresentation.h"

static uint qHash(pqSpreadSheetViewModel::vtkIndex index)
{
  return qHash(index.Tuple[2]);
}


class pqSpreadSheetViewModel::pqInternal
{
public:
  pqInternal(pqSpreadSheetViewModel* svmodel) : NumberOfColumns(0), NumberOfRows(0),
  SelectionModel(svmodel)
  {
  this->ActiveBlockNumber = 0;
  this->Dirty = true;
  this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  }

  QPointer<pqDataRepresentation> DataRepresentation;
  vtkSmartPointer<vtkSMSpreadSheetRepresentationProxy> Representation;
  int NumberOfColumns;
  int NumberOfRows;
  QItemSelectionModel SelectionModel;
  vtkIdType ActiveBlockNumber;

  vtkIdType getBlockSize()
    {
    vtkIdType blocksize = pqSMAdaptor::getElementProperty(
      this->Representation->GetProperty("BlockSize")).value<vtkIdType>();
    return blocksize;
    }

  // Computes the block number given the row number.
  vtkIdType computeBlockNumber(int row)
    {
    vtkIdType blocksize = this->getBlockSize();
    return row/blocksize;
    }

  // Computes the index for the row. 
  vtkIdType computeBlockOffset(int row)
    {
    vtkIdType blocksize = this->getBlockSize(); 
    return (row % blocksize);
    }

  // Given the offset for a location in the current block,
  // this computes the row number for that location.
  int computeRowIndex(vtkIdType blockOffset)
    {
    vtkIdType blocksize = this->getBlockSize(); 
    vtkIdType blockNumber = this->ActiveBlockNumber; 
    return (blocksize*blockNumber + blockOffset);
    }

  int getFieldType()
    {
    return pqSMAdaptor::getElementProperty(
      this->Representation->GetProperty("FieldType")).toInt();
    }

  QTimer Timer;
  QSet<vtkIdType> PendingBlocks;

  QTimer SelectionTimer;
  QSet<vtkIdType> PendingSelectionBlocks;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;

  bool Dirty;
};

//-----------------------------------------------------------------------------
pqSpreadSheetViewModel::pqSpreadSheetViewModel()
{
  this->Internal = new pqInternal(this);
  
  this->Internal->Timer.setSingleShot(true);
  this->Internal->Timer.setInterval(500);//milliseconds.
  QObject::connect(&this->Internal->Timer, SIGNAL(timeout()),
    this, SLOT(delayedUpdate()));

  this->Internal->SelectionTimer.setSingleShot(true);
  this->Internal->SelectionTimer.setInterval(100);//milliseconds.
  QObject::connect(&this->Internal->SelectionTimer, SIGNAL(timeout()),
    this, SLOT(delayedSelectionUpdate()));
}

//-----------------------------------------------------------------------------
pqSpreadSheetViewModel::~pqSpreadSheetViewModel()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
int pqSpreadSheetViewModel::rowCount(const QModelIndex&) const
{
  return this->Internal->NumberOfRows;
}

//-----------------------------------------------------------------------------
int pqSpreadSheetViewModel::columnCount(const QModelIndex&) const
{
  return this->Internal->NumberOfColumns;
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::setRepresentation(pqDataRepresentation* repr)
{
  this->Internal->DataRepresentation = repr;
  this->setRepresentationProxy(repr? 
    vtkSMSpreadSheetRepresentationProxy::SafeDownCast(repr->getProxy()) : 0);
}

//-----------------------------------------------------------------------------
pqDataRepresentation* pqSpreadSheetViewModel::getRepresentation() const
{
  return this->Internal->DataRepresentation;
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::setRepresentationProxy(
  vtkSMSpreadSheetRepresentationProxy* repr)
{
  if (this->Internal->Representation.GetPointer() != repr)
    {
    this->Internal->VTKConnect->Disconnect();
    this->Internal->Representation = repr;
    this->Internal->Dirty = true;
    if (repr)
      {
      // when repr updates, the view is dirty.
      this->Internal->VTKConnect->Connect(repr, vtkCommand::EndEvent,
        this, SLOT(markDirty()));
      this->Internal->VTKConnect->Connect(repr, vtkCommand::PropertyModifiedEvent,
        this, SLOT(markDirty()));
      }
    }
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::markDirty()
{
  this->Internal->Dirty = true;
}

//-----------------------------------------------------------------------------
vtkSMSpreadSheetRepresentationProxy* pqSpreadSheetViewModel::
getRepresentationProxy() const
{
  return this->Internal->Representation;
}

//-----------------------------------------------------------------------------
int pqSpreadSheetViewModel::getFieldType() const
{
  if (this->Internal->Representation)
    {
    return this->Internal->getFieldType();
    }
  return -1;
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::update()
{
  if (this->Internal->Dirty)
    {
    this->forceUpdate();
    }
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::forceUpdate()
{
  this->Internal->Dirty = false;
  // Note that this method is called after the representation has already been
  // updated.
  int old_rows = this->Internal->NumberOfRows;
  int old_columns = this->Internal->NumberOfColumns;

  this->Internal->NumberOfRows = 0;
  this->Internal->NumberOfColumns = 0;
  vtkSMSpreadSheetRepresentationProxy* repr = this->Internal->Representation;
  if (repr)
    {
    vtkTable* table = vtkTable::SafeDownCast(
      repr->GetOutput(this->Internal->ActiveBlockNumber));
    this->Internal->NumberOfRows = repr->GetMaximumNumberOfItems();
    this->Internal->NumberOfColumns = table? table->GetNumberOfColumns()  :0;
    if (this->Internal->NumberOfColumns == 0 && this->Internal->ActiveBlockNumber != 0)
      {
      // it is possible that the current index is invalid (data size may have
      // shrunk), update the view once again.
      this->Internal->ActiveBlockNumber = 0;
      this->forceUpdate();
      }
    
    // When SelectionOnly is true, the delivered data has an extra
    // "vtkOriginalIndices" column that needs to be hidden since it does not
    // make any sense to the user.
    if (this->Internal->NumberOfColumns && repr->GetSelectionOnly())
      {
      this->Internal->NumberOfColumns--;
      }
    }

  this->Internal->SelectionModel.clear();
  emit this->selectionChanged(this->Internal->SelectionModel.selection());

  if (old_rows == this->Internal->NumberOfRows &&
    old_columns == this->Internal->NumberOfColumns)
    {
    this->Internal->SelectionTimer.start();
    this->Internal->Timer.start();
    }
  else
    {
    this->reset();
    }
  
  // We do not fetch any data just yet. All data fetches happen when we want to
  // show the data on the GUI.
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::updateSelectionForBlock(vtkIdType blockNumber)
{
  vtkSMSpreadSheetRepresentationProxy* repr = this->Internal->Representation;
  if (repr && 
    this->Internal->getFieldType() != vtkIndexBasedBlockFilter::FIELD)
    {
    // If we are showing only the selected items, then there's not point in
    // highlighting the selected items, since all items are selected. So we
    // don't do any selection highlighting if SelectionOnly is true.
    if (repr->GetSelectionOnly())
      {
      this->Internal->SelectionModel.clear();
      }
    else
      {
      vtkSelection* selection = repr->GetSelectionOutput(blockNumber);
      // This selection has information about ids that are currently selected.
      // We now need to create a Qt selection list of indices for the items in
      // the vtk selection.
      QItemSelection qtSelection = this->convertToQtSelection(selection);
      this->Internal->SelectionModel.select(qtSelection,
        QItemSelectionModel::Select|QItemSelectionModel::Rows|
        QItemSelectionModel::Clear);
      }
    emit this->selectionChanged(this->Internal->SelectionModel.selection());
    }
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::delayedUpdate()
{
  vtkSMSpreadSheetRepresentationProxy* repr = 
    this->Internal->Representation;
  if (repr)
    {
    QModelIndex topLeft;
    QModelIndex bottomRight;
    vtkIdType blocksize = this->Internal->getBlockSize();
    foreach (vtkIdType blockNumber, this->Internal->PendingBlocks)
      {
      // cout << "Requesting : (" << repr << ") " << blockNumber << endl;
      this->Internal->ActiveBlockNumber = blockNumber;
      repr->GetOutput(this->Internal->ActiveBlockNumber);

      QModelIndex myTopLeft(this->index(blockNumber*blocksize, 0));
      int botRow = blocksize*(blockNumber+1);
      botRow = (botRow<this->rowCount())? botRow: this->rowCount()-1;
      QModelIndex myBottomRight(this->index(botRow, this->columnCount()-1));
      topLeft = (topLeft.isValid() && topLeft < myTopLeft)?  topLeft : myTopLeft;
      bottomRight = (bottomRight.isValid() && myBottomRight<bottomRight)? 
        bottomRight:myBottomRight;
      }
    if (topLeft.isValid() && bottomRight.isValid())
      {
      this->dataChanged(topLeft, bottomRight);

      // we always invalidate header data, just to be on a safe side.
      this->headerDataChanged(Qt::Horizontal, 0, this->columnCount()-1);
      }
    }
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::delayedSelectionUpdate()
{
  vtkSMSpreadSheetRepresentationProxy* repr = 
    this->Internal->Representation;
  if (repr)
    {
    foreach (vtkIdType blockNumber, this->Internal->PendingSelectionBlocks)
      {
      // we grow the current selection.
      this->Internal->ActiveBlockNumber = blockNumber;
      this->updateSelectionForBlock(blockNumber);
      }
    }
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewModel::setActiveBlock(QModelIndex top, QModelIndex bottom)
{
  this->Internal->PendingBlocks.clear();
  this->Internal->PendingSelectionBlocks.clear();
  if (this->Internal->Representation)
    {
    vtkIdType topBlock = this->Internal->computeBlockNumber(top.row());
    vtkIdType bottomBlock = this->Internal->computeBlockNumber(bottom.row());
    for (vtkIdType cc=topBlock; cc <= bottomBlock; cc++)
      {
      this->Internal->PendingBlocks.insert(cc);
      this->Internal->PendingSelectionBlocks.insert(cc);
      }
    }
}

//-----------------------------------------------------------------------------
QVariant pqSpreadSheetViewModel::data(
  const QModelIndex& idx, int role/*=Qt::DisplayRole*/) const
{
  vtkSMSpreadSheetRepresentationProxy* repr = 
    this->Internal->Representation;
  int row = idx.row();
  int column = idx.column();
  if (role == Qt::DisplayRole && repr)
    {
    vtkIdType blockNumber = this->Internal->computeBlockNumber(row);
    vtkIdType blockOffset = this->Internal->computeBlockOffset(row);
    // cout << row << " " << "blockNumber: " << blockNumber << endl;
    // cout << row << " " << "blockOffset: " << blockOffset << endl;

    if (!repr->IsAvailable(blockNumber)) // FIXME: 
                                  // show we also check for selection block 
                                  // availability here?
      {
      this->Internal->Timer.start();
      return QVariant("...");
      }

    // If displaying field data, check to make sure that the data is valid
    // since its arrays can be of different lengths
    if(this->Internal->getFieldType() == vtkIndexBasedBlockFilter::FIELD)
      {
      if(!this->isDataValid(idx))
        {
        return QVariant("");
        }
      }

    if (!repr->IsSelectionAvailable(blockNumber))
      {
      this->Internal->SelectionTimer.start();
      }

    this->Internal->ActiveBlockNumber = blockNumber;
    vtkTable* table = vtkTable::SafeDownCast(repr->GetOutput(blockNumber));
    if (table)
      {
      vtkVariant value = table->GetValue(blockOffset, column);
      QString str = value.ToString().c_str();
      if (value.IsChar() || value.IsUnsignedChar() || value.IsSignedChar())
        {
        // Don't show ASCII characted for char arrays.
        str = QString::number(value.ToInt());
        }
      str.replace(" ", "\t");
      return str;
      }
    }

  return QVariant();
}

//-----------------------------------------------------------------------------
QVariant pqSpreadSheetViewModel::headerData (int section, Qt::Orientation orientation, 
    int role/*=Qt::DisplayRole*/) const 
{
  vtkSMSpreadSheetRepresentationProxy* repr = 
    this->Internal->Representation;
  if (orientation == Qt::Horizontal && repr && role == Qt::DisplayRole)
    {
    if (!repr->IsAvailable(this->Internal->ActiveBlockNumber))
      {
      // Generally, this case doesn't arise since header data is invalidated in
      // forceUpdate() only after the data for the active block has been
      // fetched.
      // However, when progress bar is begin painted, Qt may call this method 
      // to paint the header on this view. In that case this method would have 
      // been called before the this->forceUpdate() was called which will 
      // ensure that the data is available.  
      // This skips such cases.
      return QVariant("...");
      }

    // No need to get updated data, simply get the current data.
    vtkTable* table = vtkTable::SafeDownCast(repr->GetOutput(this->Internal->ActiveBlockNumber));
    if (table && table->GetNumberOfColumns() > section)
      {
      QString title = table->GetColumnName(section);
      // Convert names of some standard arrays to user-friendly ones.
      if (title == "vtkOriginalProcessIds")
        {
        title = "Process ID";
        }
      else if (title == "vtkOriginalIndices")
        {
        title = (this->Internal->getFieldType() == vtkIndexBasedBlockFilter::POINT)?
          "Point ID" : "Cell ID";
        }
      else if (title == "vtkOriginalCellIds" && repr->GetSelectionOnly())
        {
        title = "Cell ID";
        }
      else if (title == "vtkOriginalPointIds" && repr->GetSelectionOnly())
        {
        title = "Point ID";
        }
      else if (title == "vtkCompositeIndexArray")
        {
        title = "Block Number";
        }

      return QVariant(title);
      }
    }
  else if (orientation == Qt::Vertical && repr && role == Qt::DisplayRole)
    {
    // Row number to start from 0.
    QVariant rowNo = this->Superclass::headerData(section, orientation, role);
    return QVariant(rowNo.toUInt()-1);
    }

  return this->Superclass::headerData(section, orientation, role);
}

//-----------------------------------------------------------------------------
QModelIndex pqSpreadSheetViewModel::indexFor(vtkSelection* vtkselection, vtkIdType vtkindex)
{
  vtkSMSpreadSheetRepresentationProxy* repr = 
    this->Internal->Representation;

  // Find the qt index for a row with given process id and original id. 
  vtkTable* activeBlock = vtkTable::SafeDownCast(
    this->Internal->Representation->GetOutput(this->Internal->ActiveBlockNumber));

  const char* column_name = "vtkOriginalIndices";
  if (repr->GetSelectionOnly())
    {
    column_name = (this->Internal->getFieldType() == vtkIndexBasedBlockFilter::POINT)?
      "vtkOriginalPointIds" : "vtkOriginalCellIds";
    }

  vtkIdTypeArray* indexcolumn = vtkIdTypeArray::SafeDownCast(
    activeBlock->GetColumnByName(column_name));

  vtkIdTypeArray* pidcolumn = vtkIdTypeArray::SafeDownCast(
    activeBlock->GetColumnByName("vtkOriginalProcessIds"));

  vtkUnsignedIntArray* compositeIndexColumn = vtkUnsignedIntArray::SafeDownCast(
    activeBlock->GetColumnByName("vtkCompositeIndexArray"));

  // Get the list of vtkOriginalIndices that match the given vtkindex.
  vtkIdList* ids = vtkIdList::New();
  indexcolumn->LookupValue(vtkindex, ids);
 
  if (vtkselection->GetProperties()->Has(vtkSelection::PROCESS_ID()) && pidcolumn)
    {
    int pid = vtkselection->GetProperties()->Get(vtkSelection::PROCESS_ID());
    if (pid != -1)
      {
      // remove those ids from the "ids" list that don't have the same process ID
      // as the selection.
      for (vtkIdType cc=0; cc < ids->GetNumberOfIds();)
        {
        vtkIdType id = ids->GetId(static_cast<int>(cc));
        if (pidcolumn->GetValue(id) != pid)
          {
          ids->DeleteId(id);
          }
        else
          {
          cc++;
          }
        }
      }
    }

  if (vtkselection->GetProperties()->Has(vtkSelection::HIERARCHICAL_LEVEL()) &&
      vtkselection->GetProperties()->Has(vtkSelection::HIERARCHICAL_INDEX()) &&
      compositeIndexColumn && compositeIndexColumn->GetNumberOfComponents() == 2)
    {
    unsigned int hid = static_cast<unsigned int>(
      vtkselection->GetProperties()->Get(vtkSelection::HIERARCHICAL_INDEX()));
    unsigned int hlevel = static_cast<unsigned int>(
      vtkselection->GetProperties()->Get(vtkSelection::HIERARCHICAL_LEVEL()));
    // remove those ids from the "ids" list that don't have the same process ID
    // as the selection.
    for (vtkIdType cc=0; cc < ids->GetNumberOfIds();)
      {
      vtkIdType id = ids->GetId(static_cast<int>(cc));
      unsigned int val[2];
      compositeIndexColumn->GetTupleValue(id, val);
      if (val[0] != hlevel || val[1] != hid)
        {
        ids->DeleteId(id);
        }
      else
        {
        cc++;
        }
      }
    }
  else if (vtkselection->GetProperties()->Has(vtkSelection::COMPOSITE_INDEX()) && 
    compositeIndexColumn)
    {
    unsigned int cid = static_cast<unsigned int>(
      vtkselection->GetProperties()->Get(vtkSelection::COMPOSITE_INDEX()));
    // remove those ids from the "ids" list that don't have the same process ID
    // as the selection.
    for (vtkIdType cc=0; cc < ids->GetNumberOfIds();)
      {
      vtkIdType id = ids->GetId(static_cast<int>(cc));
      if (compositeIndexColumn->GetValue(id) != cid)
        {
        ids->DeleteId(id);
        }
      else
        {
        cc++;
        }
      }
    }

  QModelIndex idx;
  if (ids->GetNumberOfIds() > 0)
    {
    if (ids->GetNumberOfIds() > 1)
      {
      qCritical() << "Multiple ids match the same selection index. Probably a BUG.";
      }
    idx = this->createIndex(this->Internal->computeRowIndex(ids->GetId(0)), 0);
    }

  ids->Delete();
  return idx;
}

//-----------------------------------------------------------------------------
QItemSelection pqSpreadSheetViewModel::convertToQtSelection(vtkSelection* vtkselection)
{
  if (!vtkselection)
    {
    return QItemSelection();
    }

  if (vtkselection->GetContentType() == vtkSelection::SELECTIONS)
    {
    QItemSelection qSel;
    for (unsigned int cc=0; cc < vtkselection->GetNumberOfChildren(); cc++)
      {
      vtkSelection* sel = vtkselection->GetChild(cc);
      qSel.merge(this->convertToQtSelection(sel), QItemSelectionModel::Select);
      }
    return qSel;
    }
  else if (vtkselection->GetContentType() == vtkSelection::INDICES)
    {
    QItemSelection qSel;
    // Iterate over all indices in the vtk selection, 
    // Determine the qt model index for each and then add that to the
    // qt selection.
    vtkIdTypeArray *indices = vtkIdTypeArray::SafeDownCast(
      vtkselection->GetSelectionList());
    for (vtkIdType cc=0; indices && cc < indices->GetNumberOfTuples(); cc++)
      {
      vtkIdType idx = indices->GetValue(cc);
      QModelIndex qtIndex = this->indexFor(vtkselection, idx);
      if (qtIndex.isValid())
        {
        // cout << "Selecting: " << qtIndex.row() << endl;
        qSel.select(qtIndex, qtIndex);
        }
      }
    return qSel;
    }
  else if (vtkselection->GetContentType() == vtkSelection::BLOCKS)
    {
    QItemSelection qSel;
    vtkUnsignedIntArray* blocks = vtkUnsignedIntArray::SafeDownCast(
      vtkselection->GetSelectionList());
    if (blocks && blocks->GetNumberOfTuples() > 0)
      {
      qSel.select(this->createIndex(0, 0),
        this->createIndex(this->rowCount()-1, 0));
      }
    return qSel;
    }
  qCritical() << "Unknown selection object.";
  return QItemSelection();

}

//-----------------------------------------------------------------------------
QSet<pqSpreadSheetViewModel::vtkIndex> pqSpreadSheetViewModel::getVTKIndices(
  const QModelIndexList& indexes)
{
  // each variant in the vtkindices is 3 tuple
  // - (-1, pid, id) or
  // - (cid, pid, id) or
  // - (hlevel, hindex, id)
  QSet<vtkIndex> vtkindices;

  vtkSMSpreadSheetRepresentationProxy* repr =
    this->getRepresentationProxy();
  if (!repr)
    {
    return vtkindices;
    }

  foreach (QModelIndex idx, indexes)
    {
    int row = idx.row();
    vtkIdType blockNumber = this->Internal->computeBlockNumber(row);
    vtkIdType blockOffset = this->Internal->computeBlockOffset(row);

    this->Internal->ActiveBlockNumber = blockNumber;
    vtkTable* table = vtkTable::SafeDownCast(repr->GetOutput(blockNumber));
    if (table)
      {
      vtkIndex value;

      vtkVariant processId = table->GetValueByName(blockOffset, "vtkOriginalProcessIds");

      const char* column_name = "vtkOriginalIndices";
      if (repr->GetSelectionOnly())
        {
        column_name = (this->Internal->getFieldType() == vtkIndexBasedBlockFilter::POINT)?
          "vtkOriginalPointIds" : "vtkOriginalCellIds";
        }

      int pid = processId.IsValid()? processId.ToInt() : -1;
      value.Tuple[1] = pid;

      vtkUnsignedIntArray* compositeIndexColumn = vtkUnsignedIntArray::SafeDownCast( 
        table->GetColumnByName("vtkCompositeIndexArray"));
      if (compositeIndexColumn)
        {
        if (compositeIndexColumn->GetNumberOfComponents() == 2)
          {
          // using hierarchical indexing.
          unsigned int val[3];
          compositeIndexColumn->GetTupleValue(blockOffset, val);
          value.Tuple[0] = static_cast<vtkIdType>(val[0]);
          value.Tuple[1] = static_cast<vtkIdType>(val[1]);
          }
        else
          {
          value.Tuple[0] = compositeIndexColumn->GetValue(blockOffset);
          }
        }

      vtkVariant vtkindex = table->GetValueByName(blockOffset, column_name);
      value.Tuple[2] = static_cast<vtkIdType>(vtkindex.ToLongLong());
      vtkindices.insert(value);
      }
    }
  return vtkindices;
}


//-----------------------------------------------------------------------------
bool pqSpreadSheetViewModel::isDataValid( const QModelIndex &idx) const
{
  // First make sure the index itself is valid
  if(!idx.isValid())
    {
    return false;
    }

  vtkSMSpreadSheetRepresentationProxy* repr = this->Internal->Representation;
  if (repr)
    {
    vtkTable* table = vtkTable::SafeDownCast(
      repr->GetOutput(this->Internal->ActiveBlockNumber));

    vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
      repr->GetProperty("Input"));
    vtkSMSourceProxy* inputProxy = vtkSMSourceProxy::SafeDownCast(
      ip->GetProxy(0));
    int port = ip->GetOutputPortForConnection(0);

    int field_type = this->Internal->getFieldType(); 

    vtkPVDataInformation* info = inputProxy?
      inputProxy->GetDataInformation(port) : 0;

    // Get the appropriate attribute information object
    vtkPVDataSetAttributesInformation *attrInfo = NULL;
    if (info)
      {
      if (field_type == vtkIndexBasedBlockFilter::FIELD)
        {
        attrInfo = info->GetFieldDataInformation();
        }
      else if (field_type == vtkIndexBasedBlockFilter::POINT)
        {
        attrInfo = info->GetPointDataInformation();
        }
      else if (field_type == vtkIndexBasedBlockFilter::CELL)
        {
        attrInfo = info->GetCellDataInformation();
        }
      }
 
    if(attrInfo)
      {
      // Ensure that the row of this index is less than the length of the 
      // data array associated with its column
      vtkPVArrayInformation *arrayInfo = attrInfo->GetArrayInformation(table->GetColumnName(idx.column()));
      if(arrayInfo && idx.row() < arrayInfo->GetNumberOfTuples())
        {
        return true;
        }
      }
    }

  return false;
}
