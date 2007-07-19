/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqComparativeVisPanel.cxx,v $

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
#include "pqComparativeVisPanel.h"
#include "ui_pqComparativeVisPanel.h"

// Server Manager Includes.
#include "vtkSMComparativeViewProxy.h"

// Qt Includes.
#include <QHeaderView>
#include <QPointer>

// ParaView Includes.
#include "pqActiveView.h"
#include "pqAnimationKeyFrame.h"
#include "pqAnimationModel.h"
#include "pqAnimationTrack.h"
#include "pqComparativeRenderView.h"
#include "pqPropertyLinks.h"
#include "pqSignalAdaptors.h"

class pqComparativeVisPanel::pqInternal : public Ui::ComparativeView
{
public:
  QPointer<pqComparativeRenderView> View;
  pqPropertyLinks Links;
  pqSignalAdaptorComboBox* ModeAdaptor;
};

//-----------------------------------------------------------------------------
pqComparativeVisPanel::pqComparativeVisPanel(QWidget* p):Superclass(p)
{
  this->Internal = new pqInternal();
  this->Internal->setupUi(this);
  this->Internal->ModeAdaptor = 
    new pqSignalAdaptorComboBox(this->Internal->Mode);
  this->Internal->Links.setUseUncheckedProperties(true);

  QObject::connect(&this->Internal->Links,
    SIGNAL(qtWidgetChanged()),
    this, SLOT(updateParameterPanel()), Qt::QueuedConnection);

  //this->Internal->AnimationWidget->header()->hide();

  QObject::connect(this->Internal->Update, SIGNAL(clicked()),
    this, SLOT(updateView()), Qt::QueuedConnection);

  // FIXME: move connection to pqMainWindowCore.
  QObject::connect(&pqActiveView::instance(), SIGNAL(changed(pqView*)),
    this, SLOT(setView(pqView*)));


  this->setEnabled(false);
}

//-----------------------------------------------------------------------------
pqComparativeVisPanel::~pqComparativeVisPanel()
{
  delete this->Internal->ModeAdaptor;
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqComparativeVisPanel::setView(pqView* view)
{
  pqComparativeRenderView* cvView = qobject_cast<pqComparativeRenderView*>(view);
  if (this->Internal->View == cvView)
    {
    return;
    }

  this->Internal->Links.removeAllPropertyLinks();
  this->Internal->View = cvView;
  if (!cvView)
    {
    this->setEnabled(false);
    return;
    }

  vtkSMComparativeViewProxy* viewProxy = cvView->getComparativeRenderViewProxy();

  this->setEnabled(true);

  this->Internal->Links.addPropertyLink(
    this->Internal->XFrames, "value", SIGNAL(valueChanged(int)),
    viewProxy, viewProxy->GetProperty("Dimensions"), 0);

  this->Internal->Links.addPropertyLink(
    this->Internal->YFrames, "value", SIGNAL(valueChanged(int)),
    viewProxy, viewProxy->GetProperty("Dimensions"), 1);

  this->Internal->Links.addPropertyLink(
    this->Internal->ModeAdaptor, "currentText", 
    SIGNAL(currentTextChanged(const QString&)),
    viewProxy, viewProxy->GetProperty("Mode"));

  this->updateParameterPanel();
}


//-----------------------------------------------------------------------------
void pqComparativeVisPanel::updateParameterPanel()
{
  /*int dx = this->Internal->XFrames->value();
  int dy = this->Internal->YFrames->value();*/
  int mode = this->Internal->ModeAdaptor->currentIndex();

  pqAnimationModel* model = this->Internal->AnimationWidget->animationModel();

  // Remove old tracks.
  while (model->count())
    {
    pqAnimationTrack* track = model->track(0);
    model->removeTrack(track);
    }

  if (mode == vtkSMComparativeViewProxy::FILM_STRIP)
    {
    pqAnimationTrack* track = model->addTrack();
    track->setProperty("Time Axis");
    }
  else
    {
    pqAnimationTrack* track = model->addTrack();
    track->setProperty("X Axis");
    track = model->addTrack();
    track->setProperty("Y Axis");
    }
}

//-----------------------------------------------------------------------------
void pqComparativeVisPanel::updateView()
{
  if (this->Internal->View)
    {
    this->Internal->Links.accept();
    this->Internal->View->render();
    }
}
