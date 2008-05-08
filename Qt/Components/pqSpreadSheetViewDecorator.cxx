/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqSpreadSheetViewDecorator.cxx,v $

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
#include "pqSpreadSheetViewDecorator.h"
#include "ui_pqSpreadSheetViewDecorator.h"

// Server Manager Includes.
#include "vtkSMProxy.h"

// Qt Includes.
#include <QVBoxLayout>
#include <QWidget>
#include <QComboBox>

// ParaView Includes.
#include "pqApplicationCore.h"
#include "pqDataRepresentation.h"
#include "pqDisplayPolicy.h"
#include "pqSignalAdaptors.h"
#include "pqSpreadSheetView.h"
#include "pqPropertyLinks.h"

class pqSpreadSheetViewDecorator::pqInternal : public Ui::pqSpreadSheetViewDecorator
{
public:
  pqPropertyLinks Links;
  pqSignalAdaptorComboBox* AttributeAdaptor;
};

//-----------------------------------------------------------------------------
pqSpreadSheetViewDecorator::pqSpreadSheetViewDecorator(pqSpreadSheetView* view):
  Superclass(view)
{
  this->Spreadsheet = view;
  QWidget* container = view->getWidget();

  QWidget* header = new QWidget(container);
  QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(container->layout());
  
  this->Internal = new pqInternal();
  this->Internal->setupUi(header);
  this->Internal->Source->setAutoUpdateIndex(false);
  this->Internal->Source->addCustomEntry("None", NULL);
  this->Internal->Source->fillExistingPorts();
  this->Internal->AttributeAdaptor = 
    new pqSignalAdaptorComboBox(this->Internal->Attribute);

  QObject::connect(&this->Internal->Links, SIGNAL(smPropertyChanged()),
    this->Spreadsheet, SLOT(render()));

  QObject::connect(this->Internal->Source, SIGNAL(currentIndexChanged(pqOutputPort*)),
    this, SLOT(currentIndexChanged(pqOutputPort*)));
  QObject::connect(this->Spreadsheet, SIGNAL(showing(pqDataRepresentation*)),
    this, SLOT(showing(pqDataRepresentation*)));

  layout->insertWidget(0, header);
  this->showing(0); //TODO: get the actual repr currently shown by the view.
}

//-----------------------------------------------------------------------------
pqSpreadSheetViewDecorator::~pqSpreadSheetViewDecorator()
{
  delete this->Internal;
  this->Internal = 0;
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewDecorator::showing(pqDataRepresentation* repr)
{
  this->Internal->Links.removeAllPropertyLinks();
  if (repr)
    {
    this->Internal->Source->setCurrentPort(repr->getOutputPortFromInput());
    vtkSMProxy* reprProxy = repr->getProxy();
    this->Internal->Links.addPropertyLink(this->Internal->AttributeAdaptor,
      "currentText", SIGNAL(currentTextChanged(const QString&)),
      reprProxy, reprProxy->GetProperty("FieldType"));
    }
  else
    {
    this->Internal->Source->setCurrentPort(NULL);
    }

  this->Internal->Attribute->setEnabled(repr != 0);
}

//-----------------------------------------------------------------------------
void pqSpreadSheetViewDecorator::currentIndexChanged(pqOutputPort* port)
{
  if (port)
    {
    pqDisplayPolicy* dpolicy = 
      pqApplicationCore::instance()->getDisplayPolicy();
    // Will create new display if needed. May also create new view 
    // as defined by the policy.
    if (dpolicy->setRepresentationVisibility(port, this->Spreadsheet, true))
      {
      this->Spreadsheet->render();
      }
    }
}
