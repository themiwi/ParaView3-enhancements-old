/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqProxyTabWidget.cxx,v $

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

=========================================================================*/

// this include
#include "pqProxyTabWidget.h"

// Qt includes
#include <QScrollArea>

// VTK includes

// ParaView Server Manager includes

// ParaView widget includes

// ParaView core includes
#include "pqDataRepresentation.h"
#include "pqOutputPort.h"
#include "pqPipelineSource.h"
#include "pqView.h"

// ParaView components includes
#include "pqObjectInspectorWidget.h"
#include "pqProxyInformationWidget.h"
#include "pqDisplayProxyEditorWidget.h"


//-----------------------------------------------------------------------------
pqProxyTabWidget::pqProxyTabWidget(QWidget* p)
  : QTabWidget(p)
{
  this->Inspector = new pqObjectInspectorWidget();
  this->addTab(this->Inspector, tr("Properties"));

  QScrollArea* scr = new QScrollArea;
  scr->setWidgetResizable(true);
  scr->setFrameShape(QFrame::NoFrame);
  this->Display = new pqDisplayProxyEditorWidget();
  scr->setWidget(this->Display);
  this->addTab(scr, tr("Display"));

  scr = new QScrollArea;
  scr->setWidgetResizable(true);
  scr->setFrameShape(QFrame::NoFrame);
  this->Information = new pqProxyInformationWidget();
  scr->setWidget(this->Information);
  this->addTab(scr, tr("Information"));

  // TODO: allow display page to work without help
  QObject::connect(this->Inspector, SIGNAL(postaccept()),
                   this->Display, SLOT(reloadGUI()));

  this->DelayedSetViewTimer.setSingleShot(true);
  this->DelayedSetViewTimer.setInterval(1);
  QObject::connect(&this->DelayedSetViewTimer, SIGNAL(timeout()),
    this, SLOT(setViewInternal()));
}

//-----------------------------------------------------------------------------
pqProxyTabWidget::~pqProxyTabWidget()
{
}

//-----------------------------------------------------------------------------
void pqProxyTabWidget::setProxy(pqPipelineSource* proxy) 
{
  this->Inspector->setProxy(proxy);
}

//-----------------------------------------------------------------------------
void pqProxyTabWidget::setView(pqView* view) 
{
  this->RequestedView = view;
  this->DelayedSetViewTimer.start();
}

//-----------------------------------------------------------------------------
void pqProxyTabWidget::setViewInternal()
{
  this->View = this->RequestedView;
  this->Inspector->setView(this->View);
  this->Display->setView(this->View);
}

//-----------------------------------------------------------------------------
void pqProxyTabWidget::setRepresentation(pqDataRepresentation* repr)
{
  this->Display->setRepresentation(repr);
}

//-----------------------------------------------------------------------------
void pqProxyTabWidget::setOutputPort(pqOutputPort* port)
{
  if (this->OutputPort == port)
    {
    return;
    }

  this->OutputPort = port;
  this->Information->setOutputPort(port);
  this->Display->setOutputPort(port);
  if (!port)
    {
    this->setRepresentation(0);
    this->setProxy(0);
    }
  else
    {
    this->setProxy(port->getSource());
    this->setRepresentation(port->getRepresentation(this->View));
    }
}

//-----------------------------------------------------------------------------
/// get the proxy for which properties are displayed
pqPipelineSource* pqProxyTabWidget::getProxy()
{
  return this->OutputPort? this->OutputPort->getSource() : 0;
}

//-----------------------------------------------------------------------------
pqObjectInspectorWidget* pqProxyTabWidget::getObjectInspector()
{
  return this->Inspector;
}


