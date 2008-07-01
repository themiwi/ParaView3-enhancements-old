/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqSphereWidget.cxx,v $

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
#include "pqSphereWidget.h"
#include "ui_pqSphereWidget.h"

// Server Manager Includes.
#include "vtkBoundingBox.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"

// Qt Includes.
#include <QDoubleValidator>

// ParaView Includes.
#include "pq3DWidgetFactory.h"
#include "pqApplicationCore.h"
#include "pqPropertyLinks.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSMSignalAdaptors.h"

class pqSphereWidget::pqImplementation : public Ui::pqSphereWidget
{
public:
  pqPropertyLinks Links;
};

#define PVSPHEREWIDGET_TRIGGER_RENDER(ui)  \
  QObject::connect(this->Implementation->ui,\
    SIGNAL(editingFinished()),\
    this, SLOT(render()), Qt::QueuedConnection);
//-----------------------------------------------------------------------------
pqSphereWidget::pqSphereWidget(vtkSMProxy* refProxy, vtkSMProxy* pxy, QWidget* _parent) :
  Superclass(refProxy, pxy, _parent)
{
  this->Implementation = new pqImplementation();
  this->Implementation->setupUi(this);
  this->Implementation->show3DWidget->setChecked(this->widgetVisible());  

  QDoubleValidator* validator = new QDoubleValidator(this);
  this->Implementation->centerX->setValidator(validator);
  this->Implementation->centerY->setValidator(validator);
  this->Implementation->centerZ->setValidator(validator);
  
  validator = new QDoubleValidator(this);
  validator->setBottom(0.0);
  this->Implementation->radius->setValidator(validator);

  PVSPHEREWIDGET_TRIGGER_RENDER(centerX);
  PVSPHEREWIDGET_TRIGGER_RENDER(centerY);
  PVSPHEREWIDGET_TRIGGER_RENDER(centerZ);
  PVSPHEREWIDGET_TRIGGER_RENDER(radius);

  QObject::connect(this->Implementation->show3DWidget,
    SIGNAL(toggled(bool)), this, SLOT(setWidgetVisible(bool)));

  QObject::connect(this, SIGNAL(widgetVisibilityChanged(bool)),
    this, SLOT(onWidgetVisibilityChanged(bool)));

  QObject::connect(this->Implementation->resetBounds,
    SIGNAL(clicked()), this, SLOT(resetBounds()));

  QObject::connect(&this->Implementation->Links, SIGNAL(qtWidgetChanged()),
    this, SLOT(setModified()));

  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();
  this->createWidget(smmodel->findServer(refProxy->GetConnectionID()));
}

//-----------------------------------------------------------------------------
pqSphereWidget::~pqSphereWidget()
{
  delete this->Implementation;
}

#define PVSPHEREWIDGET_LINK(ui, smproperty, index)\
{\
  pqSignalAdaptorDouble* adaptor = new pqSignalAdaptorDouble(\
    this->Implementation->ui, "text",\
    SIGNAL(textChanged(const QString&)));\
  this->Implementation->Links.addPropertyLink(\
    adaptor, "value", SIGNAL(valueChanged(const QString&)),\
    widget, widget->GetProperty(smproperty), index);\
}

//-----------------------------------------------------------------------------
void pqSphereWidget::createWidget(pqServer* server)
{
  vtkSMNewWidgetRepresentationProxy* widget =
    pqApplicationCore::instance()->get3DWidgetFactory()->
    get3DWidget("SphereWidgetRepresentation", server);
  this->setWidgetProxy(widget);

  widget->UpdateVTKObjects();
  widget->UpdatePropertyInformation();

  PVSPHEREWIDGET_LINK(centerX, "Center", 0);
  PVSPHEREWIDGET_LINK(centerY, "Center", 1);
  PVSPHEREWIDGET_LINK(centerZ, "Center", 2);
  PVSPHEREWIDGET_LINK(radius, "Radius", 0);
}

//-----------------------------------------------------------------------------
void pqSphereWidget::cleanupWidget()
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  if(widget)
    {
    pqApplicationCore::instance()->get3DWidgetFactory()->
      free3DWidget(widget);
    }
  this->setWidgetProxy(0);
}

//-----------------------------------------------------------------------------
void pqSphereWidget::onWidgetVisibilityChanged(bool visible)
{
  this->Implementation->show3DWidget->blockSignals(true);
  this->Implementation->show3DWidget->setChecked(visible);
  this->Implementation->show3DWidget->blockSignals(false);
}

//-----------------------------------------------------------------------------
void pqSphereWidget::resetBounds()
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  double input_bounds[6];
  if (!widget || !this->getReferenceInputBounds(input_bounds))
    {
    return;
    }


  vtkBoundingBox box;
  box.SetBounds(input_bounds);
  double center[3];
  box.GetCenter(center);

  vtkSMPropertyHelper(widget, "PlaceWidget").Set(input_bounds, 6);
  vtkSMPropertyHelper(widget, "Center").Set(center, 3);
  vtkSMPropertyHelper(widget, "Radius").Set(box.GetMaxLength()/2.0);
  widget->UpdateVTKObjects();

  this->setModified();
  this->render();
}

//-----------------------------------------------------------------------------
void pqSphereWidget::accept()
{
  this->Superclass::accept();
}

//-----------------------------------------------------------------------------
void pqSphereWidget::reset()
{
  this->Superclass::reset();
}

