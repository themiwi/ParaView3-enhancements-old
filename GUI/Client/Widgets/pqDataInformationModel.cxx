/*=========================================================================

   Program:   ParaQ
   Module:    $RCSfile: pqDataInformationModel.cxx,v $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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
#include "pqDataInformationModel.h"

// ParaView includes.
#include "vtkSMSourceProxy.h"
#include "vtkPVDataInformation.h"

// Qt includes.
#include <QList>
#include <QPointer>
#include <QtAlgorithms>
#include <QtDebug>
#include <QIcon>

// ParaQ includes.
#include "pqPipelineSource.h"

struct pqSourceInfo
{
  QPointer<pqPipelineSource> Source;
  unsigned long MTime;
  pqSourceInfo()
    {
    this->MTime = 0;
    }
  pqSourceInfo(pqPipelineSource* src)
    {
    this->Source = src;
    this->MTime = 0;
    }
  pqSourceInfo(const pqSourceInfo& info)
    {
    this->Source = info.Source;
    this->MTime = info.MTime;
    }
  operator pqPipelineSource*() const
    {
    return this->Source;
    }
  pqSourceInfo& operator=(pqPipelineSource* src)
    {
    this->Source = src;
    this->MTime = 0;
    return *this;
    }
};

//-----------------------------------------------------------------------------
class pqDataInformationModelInternal 
{
public:
  QList<pqSourceInfo > Sources;
  vtkTimeStamp UpdateTime;

  // Given a data type ID, returns the string.
  QString getDataTypeAsString(int type)
    {
    switch (type)
      {
    case -1:
      return "Unavailable";

    case VTK_POLY_DATA:
      return "Polygonal";

    case VTK_HYPER_OCTREE:
      return "Octree";

    case VTK_UNSTRUCTURED_GRID:
      return "Unstructured Grid";

    case VTK_STRUCTURED_GRID:
      return "Structured Grid";

    case VTK_RECTILINEAR_GRID:
      return "Rectilinear Grid";

    case VTK_IMAGE_DATA:
      /*
        {
        int *ext = dataInfo->GetExtent();
        if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5])
          {
          return "Image (Uniform Rectilinear)";
          }
        return "Volume (Uniform Rectilinear)";
        }
        */
      return "Image (Uniform Rectilinear)";
    case VTK_MULTIGROUP_DATA_SET:
      return "Multi-group";

    case VTK_MULTIBLOCK_DATA_SET:
      return "Multi-block";

    case VTK_HIERARCHICAL_DATA_SET:
      return "Hierarchical AMR";

    case VTK_HIERARCHICAL_BOX_DATA_SET:
      return "Hierarchical Uniform AMR";

    default:
      return "Unknown";
      }
    }

  // Given a datatype, returns the icon for that data type.
  QIcon getDataTypeAsIcon(int type)
    {
    switch (type)
      {
    case VTK_POLY_DATA:
      return QIcon(":/pqWidgets/pqPolydata16.png");

    case VTK_HYPER_OCTREE:
      return QIcon(":/pqWidgets/pqOctreeData16.png");

    case VTK_UNSTRUCTURED_GRID:
      return QIcon(":/pqWidgets/pqUnstructuredGrid16.png");

    case VTK_STRUCTURED_GRID:
      return QIcon(":/pqWidgets/pqStructuredGrid16.png");

    case VTK_RECTILINEAR_GRID:
      return QIcon(":/pqWidgets/pqRectilinearGrid16.png");

    case VTK_IMAGE_DATA:
      /*
      {
      int *ext = dataInfo->GetExtent();
      if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5])
      {
      return "Image (Uniform Rectilinear)";
      }
      return "Volume (Uniform Rectilinear)";
      }
      */
      return QIcon(":/pqWidgets/pqImageData16.png");

    case VTK_MULTIGROUP_DATA_SET:
      return QIcon(":/pqWidgets/pqMultiGroupData16.png");

    case VTK_MULTIBLOCK_DATA_SET:
      return QIcon(":/pqWidgets/pqMultiBlockData16.png");

    case VTK_HIERARCHICAL_DATA_SET:
      return QIcon(":/pqWidgets/pqHierarchicalData16.png");

    case VTK_HIERARCHICAL_BOX_DATA_SET:
      return QIcon(":/pqWidgets/pqUniformData16.png");

    default:
      return QIcon(":/pqWidgets/pqUnknownData16.png");
      }
    }
};

//-----------------------------------------------------------------------------
pqDataInformationModel::pqDataInformationModel(QObject* _parent/*=NULL*/)
  : QAbstractTableModel(_parent)
{
  this->Internal = new pqDataInformationModelInternal();
}

//-----------------------------------------------------------------------------
pqDataInformationModel::~pqDataInformationModel()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
int pqDataInformationModel::rowCount(
  const QModelIndex& vtkNotUsed(parent) /*=QModelIndex()*/) const
{
  return (this->Internal->Sources.size());
}

//-----------------------------------------------------------------------------
int pqDataInformationModel::columnCount(
  const QModelIndex &vtkNotUsed(parent) /*= QModelIndex()*/) const
{
  return 5;
}


//-----------------------------------------------------------------------------
QVariant pqDataInformationModel::data(const QModelIndex&idx, 
  int role /*= Qt::DisplayRole*/) const
{
  if (!idx.isValid() || idx.model() != this)
    {
    return QVariant();
    }

  if (idx.row() >= this->Internal->Sources.size())
    {
    qDebug() << "pqDataInformationModel::data called with invalid index: " 
      << idx.row();
    return QVariant();
    }
  if (role == Qt::ToolTipRole)
    {
    return this->headerData(idx.column(), Qt::Horizontal, Qt::DisplayRole);
    }

  pqSourceInfo &info = this->Internal->Sources[idx.row()];
  pqPipelineSource* source = info.Source;
  vtkPVDataInformation* dataInfo = 0;
  int dataType = -1;

  vtkSMSourceProxy* sourceProxy = vtkSMSourceProxy::SafeDownCast(
    source->getProxy());

  // Get data information only if the proxy is created.
  if (sourceProxy && sourceProxy->GetNumberOfParts() && 
    idx.column() > pqDataInformationModel::Name)
    {
    dataInfo = sourceProxy->GetDataInformation();
    info.MTime = dataInfo->GetMTime();
    dataType = dataInfo->GetDataSetType();
    if (dataInfo->GetCompositeDataSetType() >= 0)
      {
      dataType = dataInfo->GetCompositeDataSetType();
      }
    }

  switch (idx.column())
    {
  case pqDataInformationModel::Name:
    // Name column.
    switch(role)
      {
    case Qt::DisplayRole:
      return QVariant(source->getProxyName());
      }
    break;

  case pqDataInformationModel::DataType:
    // Data column.
    switch(role)
      {
    case Qt::DisplayRole:
      return QVariant(this->Internal->getDataTypeAsString(dataType));

    case Qt::DecorationRole:
      return QVariant(this->Internal->getDataTypeAsIcon(dataType));
      }
    break;

  case pqDataInformationModel::CellCount:
    // Number of cells.
    switch(role)
      {
    case Qt::DisplayRole:
      return (dataInfo? QVariant(static_cast<unsigned int>(
          dataInfo->GetNumberOfCells())) : QVariant("Unavailable"));

    case Qt::DecorationRole:
      return QVariant(QIcon(":/pqWidgets/pqCellData16.png"));
      }
    break;

  case pqDataInformationModel::PointCount:
    // Number of Points.
    switch (role)
      {
    case Qt::DisplayRole:
      return dataInfo? QVariant(static_cast<unsigned int>(
          dataInfo->GetNumberOfPoints())) : QVariant("Unavailable");

    case Qt::DecorationRole:
      return QVariant(QIcon(":/pqWidgets/pqPointData16.png"));
      }
    break;

  case pqDataInformationModel::MemorySize:
    // Memory.
    switch(role)
      {
    case Qt::DisplayRole:
      return dataInfo? QVariant(dataInfo->GetMemorySize()/1000.0)
        : QVariant("Unavailable");
      }
    break;

    }
  return QVariant();
}

//-----------------------------------------------------------------------------
QVariant pqDataInformationModel::headerData(int section, 
  Qt::Orientation orientation, int role /*=Qt::DisplayRole*/) const
{
  if (orientation == Qt::Horizontal)
    {
    switch(role)
      {
    case Qt::DisplayRole:
      switch (section)
        {
      case pqDataInformationModel::Name:
        return QVariant("Name");

      case pqDataInformationModel::DataType:
        return QVariant("Data");

      case pqDataInformationModel::CellCount:
        return QVariant("No. of Cells");

      case pqDataInformationModel::PointCount:
        return QVariant("No. of Points");

      case pqDataInformationModel::MemorySize:
        return QVariant("Memory (MB)");
        }
      break;
      }
    }
  return QVariant();
}

//-----------------------------------------------------------------------------
void pqDataInformationModel::addSource(pqPipelineSource* source)
{
  if (this->Internal->Sources.contains(source))
    {
    return;
    }
  this->beginInsertRows(QModelIndex(), this->Internal->Sources.size(),
    this->Internal->Sources.size());

  this->Internal->Sources.push_back(source);

  this->endInsertRows();
}

//-----------------------------------------------------------------------------
void pqDataInformationModel::removeSource(pqPipelineSource* source)
{
  int idx = this->Internal->Sources.indexOf(source);
  if (idx != -1)
    {
    this->beginRemoveRows(QModelIndex(), idx, idx);
    this->Internal->Sources.removeAt(idx);
    this->endRemoveRows();
    }
}

//-----------------------------------------------------------------------------
void pqDataInformationModel::refreshModifiedData()
{
  QList<pqSourceInfo>::iterator iter;
  int row_no = 0;
  for (iter = this->Internal->Sources.begin(); 
    iter != this->Internal->Sources.end(); ++iter, row_no++)
    {
    vtkSMSourceProxy* proxy = vtkSMSourceProxy::SafeDownCast(
      iter->Source->getProxy());
    if (!proxy)
      {
      continue;
      }
    if (proxy->GetDataInformation()->GetMTime() > iter->MTime)
      {
      emit this->dataChanged(this->index(row_no, 0),
        this->index(row_no, 4));
      }
    }
}

//-----------------------------------------------------------------------------
QModelIndex pqDataInformationModel::getIndexFor(pqPipelineSource* item) const
{
  if (!this->Internal->Sources.contains(item))
    {
    return QModelIndex();
    }
  return this->index(this->Internal->Sources.indexOf(item), 0);
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqDataInformationModel::getItemFor(const QModelIndex& idx) const
{
  if (!idx.isValid() && idx.model() != this)
    {
    return NULL;
    }
  if (idx.row() >= this->Internal->Sources.size())
    {
    qDebug() << "Index: " << idx.row() << " beyond range.";
    return NULL;
    }
  return this->Internal->Sources[idx.row()];
}

