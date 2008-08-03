/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqProxySILModel.cxx,v $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#include "pqProxySILModel.h"

// Server Manager Includes.

// Qt Includes.
#include <QApplication>
#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QStyleOptionButton>

// ParaView Includes.
#include "pqSILModel.h"

//-----------------------------------------------------------------------------
pqProxySILModel::pqProxySILModel(const QString& hierarchyName, QObject* _parent):
  Superclass(_parent)
{
  this->HierarchyName = hierarchyName;

  QStyle::State styleOptions [3] = {
    QStyle::State_On | QStyle::State_Enabled,
    QStyle::State_NoChange | QStyle::State_Enabled,
    QStyle::State_Off | QStyle::State_Enabled
  };

  QStyleOptionButton option;
  QRect r = QApplication::style()->subElementRect(QStyle::SE_CheckBoxIndicator, 
    &option);
  option.rect = QRect(QPoint(0,0), r.size());
  for(int i=0; i<3 ; i++)
    {
    this->CheckboxPixmaps[i] = QPixmap(r.size());
    this->CheckboxPixmaps[i].fill(QColor(0,0,0,0));
    QPainter painter(&this->CheckboxPixmaps[i]);
    option.state = styleOptions[i];
    QApplication::style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &option, 
      &painter);
    }

  this->DelayedValuesChangedSignalTimer.setInterval(10);
  this->DelayedValuesChangedSignalTimer.setSingleShot(true);
  QObject::connect(&this->DelayedValuesChangedSignalTimer, SIGNAL(timeout()),
    this, SIGNAL(valuesChanged()));
}

//-----------------------------------------------------------------------------
pqProxySILModel::~pqProxySILModel()
{
}

//-----------------------------------------------------------------------------
QModelIndex pqProxySILModel::mapFromSource(const QModelIndex& sourceIndex) const
{
  pqSILModel* silModel = qobject_cast<pqSILModel*>(this->sourceModel());
  if (!sourceIndex.isValid() || 
    sourceIndex == silModel->hierarchyIndex(this->HierarchyName))
    {
    return QModelIndex();
    }

  return this->createIndex(sourceIndex.row(), sourceIndex.column(),
    static_cast<quint32>(static_cast<vtkIdType>(sourceIndex.internalId())));
}

//-----------------------------------------------------------------------------
QModelIndex pqProxySILModel::mapToSource(const QModelIndex& proxyIndex) const
{
  pqSILModel* silModel = qobject_cast<pqSILModel*>(this->sourceModel());
  if (proxyIndex.isValid())
    {
    return silModel->makeIndex(static_cast<vtkIdType>(proxyIndex.internalId()));
    }

  return silModel->hierarchyIndex(this->HierarchyName);
}

//-----------------------------------------------------------------------------
void pqProxySILModel::setSourceModel(QAbstractItemModel *srcModel)
{
  if (this->sourceModel() == srcModel)
    {
    return;
    }
  if (this->sourceModel())
    {
    QObject::disconnect(this->sourceModel(), 0, this, 0);
    }

  this->Superclass::setSourceModel(srcModel);

  if (srcModel)
    {
    QObject::connect(
      srcModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
      this, SLOT(sourceDataChanged(const QModelIndex&, const QModelIndex&)));
    QObject::connect(srcModel, SIGNAL(modelReset()),
      this, SIGNAL(modelReset()));
    QObject::connect(srcModel, SIGNAL(modelAboutToBeReset()),
      this, SIGNAL(modelAboutToBeReset()));
    QObject::connect(srcModel, SIGNAL(checkStatusChanged()), 
      this, SLOT(onCheckStatusChanged()));
    }
}

//-----------------------------------------------------------------------------
QList<QVariant> pqProxySILModel::values() const
{
  pqSILModel* silModel = qobject_cast<pqSILModel*>(this->sourceModel());
  return silModel->status(this->HierarchyName);
}

//-----------------------------------------------------------------------------
void pqProxySILModel::setValues(const QList<QVariant>& arg)
{
  pqSILModel* silModel = qobject_cast<pqSILModel*>(this->sourceModel());
  silModel->setStatus(this->HierarchyName, arg);
}

//-----------------------------------------------------------------------------
void pqProxySILModel::onCheckStatusChanged()
{
  this->DelayedValuesChangedSignalTimer.start();
}

//-----------------------------------------------------------------------------
QVariant pqProxySILModel::headerData (int, Qt::Orientation, int role /*= Qt::DisplayRole*/) const
{
  if (role == Qt::DisplayRole)
    {
    return this->HierarchyName;
    }
  else if (role == Qt::DecorationRole)
    {
    QModelIndex srcIndex = this->mapToSource(QModelIndex());
    Qt::ItemFlags iflags = this->sourceModel()->flags(srcIndex);
    if ((iflags & Qt::ItemIsUserCheckable) || (iflags & Qt::ItemIsTristate))
      {
      int checkState = this->sourceModel()->data(srcIndex, Qt::CheckStateRole).toInt();
      switch (checkState)
        {
      case Qt::Checked:
        return QVariant(this->CheckboxPixmaps[0]);

      case Qt::PartiallyChecked:
        return QVariant(this->CheckboxPixmaps[1]);

      default:
        return QVariant(this->CheckboxPixmaps[2]);
        }
      }
    }

  return QVariant();
}

//-----------------------------------------------------------------------------
void pqProxySILModel::toggleRootCheckState()
{
  int checkState = this->data(QModelIndex(), Qt::CheckStateRole).toInt();
  if (checkState == Qt::PartiallyChecked || checkState == Qt::Unchecked)
    {
    this->setData(QModelIndex(), Qt::Checked, Qt::CheckStateRole);
    }
  else
    {
    this->setData(QModelIndex(), Qt::Unchecked, Qt::CheckStateRole);
    }
}
