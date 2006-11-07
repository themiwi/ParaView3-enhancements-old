/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqDisplayRepresentationWidget.cxx,v $

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
#include "pqDisplayRepresentationWidget.h"
#include "ui_pqDisplayRepresentationWidget.h"

#include "vtkSMIntVectorProperty.h"
#include "vtkSMDataObjectDisplayProxy.h"


#include<QPointer>

#include "pqPipelineSource.h"
#include "pqRenderModule.h"
#include "pqPipelineDisplay.h"
#include "pqPropertyLinks.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"

class pqDisplayRepresentationWidgetInternal : 
  public Ui::displayRepresentationWidget
{
public:
  QPointer<pqRenderModule> RenderModule;
  QPointer<pqPipelineSource> Source;
  QPointer<pqPipelineDisplay> Display;
  pqPropertyLinks Links;
  pqSignalAdaptorComboBox* Adaptor;
};

//-----------------------------------------------------------------------------
pqDisplayRepresentationWidget::pqDisplayRepresentationWidget(
  QWidget* _p): QWidget(_p)
{
  this->Internal = new pqDisplayRepresentationWidgetInternal;
  this->Internal->setupUi(this);
  this->Internal->Adaptor = new pqSignalAdaptorComboBox(
    this->Internal->comboBox);
  this->Internal->Adaptor->setObjectName("adaptor");

  QObject::connect(this->Internal->Adaptor, 
    SIGNAL(currentTextChanged(const QString&)),
    this, SLOT(onCurrentTextChanged(const QString&)), Qt::QueuedConnection);

  QObject::connect(this->Internal->Adaptor, 
    SIGNAL(currentTextChanged(const QString&)),
    this, SIGNAL(currentTextChanged(const QString&)), Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
pqDisplayRepresentationWidget::~pqDisplayRepresentationWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqDisplayRepresentationWidget::setRenderModule(pqRenderModule* rm)
{
  if (this->Internal->RenderModule)
    {
    QObject::disconnect(this->Internal->RenderModule, 0, this, 0);
    }
  this->Internal->RenderModule = rm;
  if (this->Internal->RenderModule)
    {
    QObject::connect(this->Internal->RenderModule, 
      SIGNAL(displayAdded(pqDisplay*)), 
      this, SLOT(reloadGUI()), Qt::QueuedConnection);
    }  
  this->updateLinks();
}

//-----------------------------------------------------------------------------
void pqDisplayRepresentationWidget::setDisplay(pqPipelineDisplay* disp)
{
  this->Internal->Display = disp;
  this->updateLinks();
}

//-----------------------------------------------------------------------------
void pqDisplayRepresentationWidget::update(pqPipelineSource* source)
{
  this->Internal->Source = source;
  this->updateLinks();
}

//-----------------------------------------------------------------------------
void pqDisplayRepresentationWidget::updateLinks()
{
  // break old links.
  this->Internal->Links.removeAllPropertyLinks();

  pqPipelineDisplay* display = this->Internal->Display;

  if (!display && this->Internal->Source && this->Internal->RenderModule)
    {
    // this widget cannot work on other consumer displays like plot displays.
    display = 
      qobject_cast<pqPipelineDisplay*>(
        this->Internal->Source->getDisplay(this->Internal->RenderModule));
    }

  this->Internal->comboBox->setEnabled(display!=0);
  this->Internal->comboBox->clear();
  if (!display)
    {
    this->Internal->comboBox->addItem("Representation");
    return;
    }

  vtkSMDataObjectDisplayProxy* displayProxy = display->getDisplayProxy();
  vtkSMProperty* repProperty = display->getProxy()->GetProperty("Representation");
  repProperty->UpdateDependentDomains();
  QList<QVariant> items = pqSMAdaptor::getEnumerationPropertyDomain(repProperty);
  foreach(QVariant item, items)
    {
    // Disable volume rendering support for now
    if (item == "Volume" && 1 /*!display->getDisplayProxy()->GetHasVolumePipeline()*/)
      {
      continue; // add volume only if volume representation is supported.
      }
    this->Internal->comboBox->addItem(item.toString());
    }

  this->Internal->Links.addPropertyLink(
    this->Internal->Adaptor, "currentText",
    SIGNAL(currentTextChanged(const QString&)),
    displayProxy, repProperty);
}

//-----------------------------------------------------------------------------
void pqDisplayRepresentationWidget::reloadGUI()
{
  this->updateLinks();
}

//-----------------------------------------------------------------------------
void pqDisplayRepresentationWidget::onCurrentTextChanged(const QString&)
{
  if (this->Internal->Display)
    {
    this->Internal->Display->renderAllViews();
    }
  if (this->Internal->RenderModule)
    {
    this->Internal->RenderModule->render();
    }
}
