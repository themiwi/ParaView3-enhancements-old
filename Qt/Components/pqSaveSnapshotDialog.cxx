/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqSaveSnapshotDialog.cxx,v $

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

========================================================================*/
#include "pqSaveSnapshotDialog.h"
#include "ui_pqSaveSnapshotDialog.h"

// Server Manager Includes.

// Qt Includes.
#include <QIntValidator>

// ParaView Includes.

class pqSaveSnapshotDialog::pqInternal : public Ui::SaveSnapshotDialog
{
public:
  double AspectRatio;
  QSize ViewSize;
  QSize AllViewsSize;
};

//-----------------------------------------------------------------------------
pqSaveSnapshotDialog::pqSaveSnapshotDialog(QWidget* _parent, 
  Qt::WindowFlags f):Superclass(_parent, f)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);
  this->Internal->AspectRatio = 1.0;

  QIntValidator *validator = new QIntValidator(this);
  validator->setBottom(50);
  this->Internal->width->setValidator(validator);

  validator = new QIntValidator(this);
  validator->setBottom(50);
  this->Internal->height->setValidator(validator);

  QObject::connect(this->Internal->ok, SIGNAL(pressed()),
    this, SLOT(accept()), Qt::QueuedConnection);
  QObject::connect(this->Internal->cancel, SIGNAL(pressed()),
    this, SLOT(reject()), Qt::QueuedConnection);

  QObject::connect(this->Internal->width, SIGNAL(editingFinished()),
    this, SLOT(onWidthEdited()));
  QObject::connect(this->Internal->height, SIGNAL(editingFinished()),
    this, SLOT(onHeightEdited()));
  QObject::connect(this->Internal->lockAspect, SIGNAL(toggled(bool)),
    this, SLOT(onLockAspectRatio(bool)));
  QObject::connect(this->Internal->selectedViewOnly, SIGNAL(toggled(bool)),
    this, SLOT(updateSize()));
}

//-----------------------------------------------------------------------------
pqSaveSnapshotDialog::~pqSaveSnapshotDialog()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqSaveSnapshotDialog::setViewSize(const QSize& view_size)
{
  this->Internal->ViewSize = view_size;
  this->updateSize();
}


//-----------------------------------------------------------------------------
void pqSaveSnapshotDialog::setAllViewsSize(const QSize& view_size)
{
  this->Internal->AllViewsSize = view_size;
  this->updateSize();
}

//-----------------------------------------------------------------------------
bool pqSaveSnapshotDialog::saveAllViews() const
{
  return (!this->Internal->selectedViewOnly->isChecked());
}

//-----------------------------------------------------------------------------
void pqSaveSnapshotDialog::updateSize() 
{
  if (this->saveAllViews())
    {
    this->Internal->width->setText(QString::number(
        this->Internal->AllViewsSize.width()));
    this->Internal->height->setText(QString::number(
        this->Internal->AllViewsSize.height()));
    }
  else
    {
    this->Internal->width->setText(QString::number(
        this->Internal->ViewSize.width()));
    this->Internal->height->setText(QString::number(
        this->Internal->ViewSize.height()));
    }

  QSize curSize = this->viewSize();
  this->Internal->AspectRatio = 
    curSize.width()/static_cast<double>(curSize.height());
}

//-----------------------------------------------------------------------------
void pqSaveSnapshotDialog::onLockAspectRatio(bool lock)
{
  if (lock)
    {
    QSize curSize = this->viewSize();
    this->Internal->AspectRatio = 
      curSize.width()/static_cast<double>(curSize.height());
    }
}

//-----------------------------------------------------------------------------
QSize pqSaveSnapshotDialog::viewSize() const
{
  return QSize(
    this->Internal->width->text().toInt(),
    this->Internal->height->text().toInt());
}

//-----------------------------------------------------------------------------
void pqSaveSnapshotDialog::onWidthEdited()
{
  if (this->Internal->lockAspect->isChecked())
    {
    this->Internal->height->setText(QString::number(
        static_cast<int>(
          this->Internal->width->text().toInt()/this->Internal->AspectRatio)));
    }
}

//-----------------------------------------------------------------------------
void pqSaveSnapshotDialog::onHeightEdited()
{
  if (this->Internal->lockAspect->isChecked())
    {
    this->Internal->width->setText(QString::number(
        static_cast<int>(
          this->Internal->height->text().toInt()*this->Internal->AspectRatio)));
    }
}

