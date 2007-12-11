/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqCubeAxesEditorDialog.cxx,v $

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
#include "pqCubeAxesEditorDialog.h"
#include "ui_pqCubeAxesEditorDialog.h"

// Server Manager Includes.
#include "vtkSmartPointer.h"
#include "vtkSMProxy.h"

// Qt Includes.

// ParaView Includes.
#include "pqApplicationCore.h"
#include "pqNamedWidgets.h"
#include "pqPropertyManager.h"
#include "pqSignalAdaptors.h"
#include "pqUndoStack.h"

class pqCubeAxesEditorDialog::pqInternal : public Ui::CubeAxesEditorDialog
{
public:
  vtkSmartPointer<vtkSMProxy> Representation;
  pqPropertyManager* PropertyManager;
  pqSignalAdaptorColor* ColorAdaptor;
  pqInternal()
    {
    this->PropertyManager = 0;
    this->ColorAdaptor = 0;
    }
  ~pqInternal()
    {
    delete this->PropertyManager;
    this->PropertyManager = 0;
    delete this->ColorAdaptor;
    }
};

//-----------------------------------------------------------------------------
pqCubeAxesEditorDialog::pqCubeAxesEditorDialog(
  QWidget *_parent/*=0*/, Qt::WindowFlags f/*=0*/):
  Superclass(_parent, f)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);
  this->Internal->ColorAdaptor = new pqSignalAdaptorColor(
    this->Internal->Color, "chosenColor",
    SIGNAL(chosenColorChanged(const QColor&)), false);

  pqUndoStack* ustack = pqApplicationCore::instance()->getUndoStack();
  if (ustack)
    {
    QObject::connect(this, SIGNAL(beginUndo(const QString&)),
      ustack, SLOT(beginUndoSet(const QString&)));
    QObject::connect(this, SIGNAL(endUndo()),
      ustack, SLOT(endUndoSet()));
    }
  QObject::connect(this->Internal->Ok, SIGNAL(clicked()),
    this, SLOT(accept()), Qt::QueuedConnection);
  QObject::connect(this->Internal->Cancel, SIGNAL(clicked()),
    this, SLOT(reject()), Qt::QueuedConnection);

}

//-----------------------------------------------------------------------------
pqCubeAxesEditorDialog::~pqCubeAxesEditorDialog()
{
  delete this->Internal;
}


//-----------------------------------------------------------------------------
void pqCubeAxesEditorDialog::setRepresentationProxy(vtkSMProxy* repr)
{
  if (this->Internal->Representation == repr)
    {
    return;
    }
  delete this->Internal->PropertyManager;
  this->Internal->PropertyManager = new pqPropertyManager(this);
  this->Internal->Representation = repr;
  if (repr)
    {
    // set up links between the property manager and the widgets.
    pqNamedWidgets::link(this, repr, this->Internal->PropertyManager);
    this->Internal->PropertyManager->registerLink(
      this->Internal->ColorAdaptor, "color", 
      SIGNAL(colorChanged(const QVariant&)),
      repr, repr->GetProperty("CubeAxesColor"));
    }
}


//-----------------------------------------------------------------------------
void pqCubeAxesEditorDialog::done(int res)
{
  if (res == QDialog::Accepted && this->Internal->PropertyManager->isModified())
    {
    emit this->beginUndo("Cube Axes Parameters");
    this->Internal->PropertyManager->accept();
    emit this->endUndo();
    }
  this->Superclass::done(res);
}
