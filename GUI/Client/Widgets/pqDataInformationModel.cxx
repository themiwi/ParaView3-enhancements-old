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
QVariant pqDataInformationModel::data(const QModelIndex&index, 
  int role /*= Qt::DisplayRole*/) const
{
  if (!index.isValid() || index.model() != this)
    {
    return QVariant();
    }

  if (index.row() >= this->Internal->Sources.size())
    {
    qDebug() << "pqDataInformationModel::data called with invalid index: " 
      << index.row();
    return QVariant();
    }
  if (role == Qt::ToolTipRole)
    {
    return this->headerData(index.column(), Qt::Horizontal, Qt::DisplayRole);
    }

  pqSourceInfo &info = this->Internal->Sources[index.row()];
  pqPipelineSource* source = info.Source;
  source->getProxy()->UpdateVTKObjects();
  vtkPVDataInformation* dataInfo = vtkSMSourceProxy::SafeDownCast(
    source->getProxy())->GetDataInformation();
  info.MTime = dataInfo->GetMTime();

  int dataType = dataInfo->GetDataSetType();
  if (dataInfo->GetCompositeDataSetType() >= 0)
    {
    dataType = dataInfo->GetCompositeDataSetType();
    }

  switch (index.column())
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
      }
    break;

  case pqDataInformationModel::CellCount:
    // Number of cells.
    switch(role)
      {
    case Qt::DisplayRole:
      return QVariant(static_cast<unsigned int>(
          dataInfo->GetNumberOfCells()));
      }
    break;

  case pqDataInformationModel::PointCount:
    // Number of Points.
    switch (role)
      {
    case Qt::DisplayRole:
      return QVariant(static_cast<unsigned int>(
          dataInfo->GetNumberOfPoints()));
      }
    break;

  case pqDataInformationModel::MemorySize:
    // Memory.
    switch(role)
      {
    case Qt::DisplayRole:
      return QVariant(dataInfo->GetMemorySize()/1000.0);
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
  int index = this->Internal->Sources.indexOf(source);
  if (index != -1)
    {
    this->beginRemoveRows(QModelIndex(), index, index);
    this->Internal->Sources.removeAt(index);
    this->endRemoveRows();
    }
}

//-----------------------------------------------------------------------------
void pqDataInformationModel::refreshModifiedData()
{
  QList<pqSourceInfo>::iterator iter;
  int row = 0;
  for (iter = this->Internal->Sources.begin(); 
    iter != this->Internal->Sources.end(); ++iter, row++)
    {
    vtkSMSourceProxy* proxy = vtkSMSourceProxy::SafeDownCast(
      iter->Source->getProxy());
    if (!proxy)
      {
      continue;
      }
    if (proxy->GetDataInformation()->GetMTime() > iter->MTime)
      {
      emit this->dataChanged(this->index(row, 0),
        this->index(row, 4));
      }
    }
}

//-----------------------------------------------------------------------------
