/*=========================================================================

   Program:   ParaQ
   Module:    $RCSfile: pqHandleWidget.cxx,v $

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

#include "pq3DWidgetFactory.h"
#include "pqApplicationCore.h"
#include "pqHandleWidget.h"
#include "pqRenderViewModule.h"
#include "pqServerManagerModel.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineSource.h"
#include "pqPipelineFilter.h"
#include "pqPropertyLinks.h"
#include "pqSMSignalAdaptors.h"

#include "ui_pqHandleWidget.h"

#include <vtkCamera.h>
#include <vtkMemberFunctionCommand.h>
#include <vtkHandleRepresentation.h>
#include <vtkProcessModule.h>
#include <vtkPVDataInformation.h>
#include <vtkRenderer.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMNew3DWidgetProxy.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkSMSourceProxy.h>

/////////////////////////////////////////////////////////////////////////
// pqHandleWidget::pqImplementation

class pqHandleWidget::pqImplementation
{
public:
  pqImplementation() :
    UI(new Ui::pqHandleWidget())
  {
  }
  
  ~pqImplementation()
  {
    delete this->UI;
  }
  
  /// Stores the Qt widgets
  Ui::pqHandleWidget* const UI;

  /// Callback object used to connect 3D widget events to member methods
  vtkSmartPointer<vtkCommand> StartDragObserver;
  /// Callback object used to connect 3D widget events to member methods
  vtkSmartPointer<vtkCommand> EndDragObserver;
  pqPropertyLinks Links;
};

/////////////////////////////////////////////////////////////////////////
// pqHandleWidget

pqHandleWidget::pqHandleWidget(QWidget* p) :
  Superclass(p),
  Implementation(new pqImplementation())
{
  this->Implementation->StartDragObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqHandleWidget::on3DWidgetStartDrag));
  this->Implementation->EndDragObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqHandleWidget::on3DWidgetEndDrag));

  this->Implementation->UI->setupUi(this);
  this->Implementation->UI->show3DWidget->setChecked(this->widgetVisible());

  QObject::connect(this->Implementation->UI->show3DWidget,
    SIGNAL(toggled(bool)), this, SLOT(setWidgetVisible(bool)));

  QObject::connect(this, SIGNAL(widgetVisibilityChanged(bool)),
    this, SLOT(onWidgetVisibilityChanged(bool)));

  QObject::connect(this->Implementation->UI->useCenterBounds,
    SIGNAL(clicked()), this, SLOT(onResetBounds()));

  QObject::connect(&this->Implementation->Links, SIGNAL(qtWidgetChanged()),
    this, SIGNAL(widgetChanged()));

  QObject::connect(&this->Implementation->Links, SIGNAL(smPropertyChanged()),
    this, SIGNAL(widgetChanged()));
}

//-----------------------------------------------------------------------------
pqHandleWidget::~pqHandleWidget()
{
  this->cleanupWidget();
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqHandleWidget::createWidget(pqServer* server)
{
  vtkSMNew3DWidgetProxy* widget =
    pqApplicationCore::instance()->get3DWidgetFactory()->
    get3DWidget("PointSourceWidgetDisplay", server);
  this->setWidgetProxy(widget);
  
  widget->UpdateVTKObjects();
  widget->UpdatePropertyInformation();

  pqSignalAdaptorDouble* adaptor = new pqSignalAdaptorDouble(
    this->Implementation->UI->worldPositionX, "text",
    SIGNAL(textChanged(const QString&)));
  this->Implementation->Links.addPropertyLink(
    adaptor, "value", SIGNAL(valueChanged(const QString&)),
    widget, widget->GetProperty("WorldPosition"), 0);

  adaptor = new pqSignalAdaptorDouble(
    this->Implementation->UI->worldPositionY, "text",
    SIGNAL(textChanged(const QString&)));
  this->Implementation->Links.addPropertyLink(
    adaptor, "value", SIGNAL(valueChanged(const QString&)),
    widget, widget->GetProperty("WorldPosition"), 1);

  adaptor = new pqSignalAdaptorDouble(
    this->Implementation->UI->worldPositionZ, "text",
    SIGNAL(textChanged(const QString&)));
  this->Implementation->Links.addPropertyLink(
    adaptor, "value", SIGNAL(valueChanged(const QString&)),
    widget, widget->GetProperty("WorldPosition"), 2);

  widget->AddObserver(vtkCommand::StartInteractionEvent,
    this->Implementation->StartDragObserver);
  widget->AddObserver(vtkCommand::EndInteractionEvent,
    this->Implementation->EndDragObserver);
}

//-----------------------------------------------------------------------------
void pqHandleWidget::cleanupWidget()
{
  this->Implementation->Links.removeAllPropertyLinks();
  vtkSMNew3DWidgetProxy* widget = this->getWidgetProxy();
  if(widget)
    {
    widget->RemoveObserver(
      this->Implementation->EndDragObserver);
    widget->RemoveObserver(
      this->Implementation->StartDragObserver);
    pqApplicationCore::instance()->get3DWidgetFactory()->
      free3DWidget(widget);
    }
  this->setWidgetProxy(0);
}

//-----------------------------------------------------------------------------
void pqHandleWidget::setControlledProxy(vtkSMProxy* proxy)
{
  if (!this->getWidgetProxy())
    {
    pqServerManagerModel* smModel = 
      pqApplicationCore::instance()->getServerManagerModel();
    this->createWidget(smModel->getServer(proxy->GetConnectionID()));
    }

  this->Superclass::setControlledProxy(proxy);
}

//-----------------------------------------------------------------------------
void pqHandleWidget::onWidgetVisibilityChanged(bool visible)
{
  this->Implementation->UI->show3DWidget->blockSignals(true);
  this->Implementation->UI->show3DWidget->setChecked(visible);
  this->Implementation->UI->show3DWidget->blockSignals(false);
}

//-----------------------------------------------------------------------------
void pqHandleWidget::resetBounds()
{
  vtkSMNew3DWidgetProxy* widget = this->getWidgetProxy();
  if(widget && this->getReferenceProxy())
    {
    if(vtkSMProxyProperty* const input_property =
      vtkSMProxyProperty::SafeDownCast(
        this->getReferenceProxy()->getProxy()->GetProperty("Input")))
      {
      if(vtkSMSourceProxy* const input_proxy = vtkSMSourceProxy::SafeDownCast(
          input_property->GetProxy(0)))
        {
        double input_bounds[6];
        input_proxy->GetDataInformation()->GetBounds(input_bounds);

        double input_origin[3];
        input_origin[0] = (input_bounds[0] + input_bounds[1]) / 2.0;
        input_origin[1] = (input_bounds[2] + input_bounds[3]) / 2.0;
        input_origin[2] = (input_bounds[4] + input_bounds[5]) / 2.0;

        if(widget)
          {
          if(vtkSMDoubleVectorProperty* const widget_position =
            vtkSMDoubleVectorProperty::SafeDownCast(
              widget->GetProperty("WorldPosition")))
            {
            widget_position->SetElements(input_origin);
            }

          widget->UpdateVTKObjects();

          pqApplicationCore::instance()->render();
          }
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqHandleWidget::onResetBounds()
{
  this->resetBounds();
}

//-----------------------------------------------------------------------------
void pqHandleWidget::on3DWidgetStartDrag()
{
  emit widgetStartInteraction();
}

//-----------------------------------------------------------------------------
void pqHandleWidget::on3DWidgetEndDrag()
{
  emit widgetEndInteraction();
}

