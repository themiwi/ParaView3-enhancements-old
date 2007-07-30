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
#include <QScrollArea>

// ParaView Includes.
#include "pqActiveView.h"
#include "pqAnimationKeyFrame.h"
#include "pqAnimationModel.h"
#include "pqAnimationTrack.h"
#include "pqApplicationCore.h"
#include "pqComparativeRenderView.h"
#include "pqPropertyLinks.h"
#include "pqServerManagerModel.h"
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

  /*
  QVBoxLayout* vboxlayout = new QVBoxLayout(this);
  vboxlayout->setSpacing(0);
  vboxlayout->setMargin(0);
  vboxlayout->setObjectName("vboxLayout");

  QWidget* container = new QWidget(this);
  container->setObjectName("scrollWidget");
  container->setSizePolicy(QSizePolicy::MinimumExpanding,
    QSizePolicy::MinimumExpanding);

  QScrollArea* s = new QScrollArea(this);
  s->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  s->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  s->setWidgetResizable(true);
  s->setObjectName("scrollArea");
  s->setFrameShape(QFrame::NoFrame);
  s->setWidget(container);
  vboxlayout->addWidget(s);
  this->Internal->setupUi(container);
  */

  this->Internal->setupUi(this);
  this->Internal->ModeAdaptor = 
    new pqSignalAdaptorComboBox(this->Internal->Mode);
  this->Internal->Links.setUseUncheckedProperties(true);

  QObject::connect(&this->Internal->Links,
    SIGNAL(qtWidgetChanged()),
    this, SLOT(updateParameterPanel()), Qt::QueuedConnection);

  QHeaderView* header = this->Internal->AnimationWidget->header();
  header->setClickable(true);
  QObject::connect(header,
    SIGNAL(sectionClicked(int)),
    this, SLOT(editPropertyToAnimate(int)), Qt::QueuedConnection);

  QObject::connect(this->Internal->Update, SIGNAL(clicked()),
    this, SLOT(updateView()), Qt::QueuedConnection);

  // FIXME: move connection to pqMainWindowCore.
  QObject::connect(&pqActiveView::instance(), SIGNAL(changed(pqView*)),
    this, SLOT(setView(pqView*)));

  this->Internal->XObject->setUpdateCurrentWithSelection(true);
  this->Internal->YObject->setUpdateCurrentWithSelection(true);
  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();

  QObject::connect(smmodel, SIGNAL(sourceAdded(pqPipelineSource*)),
    this->Internal->XObject, SLOT(addSource(pqPipelineSource*)));
  QObject::connect(smmodel, SIGNAL(preSourceRemoved(pqPipelineSource*)),
    this->Internal->XObject, SLOT(removeSource(pqPipelineSource*)));

  QObject::connect(smmodel, SIGNAL(sourceAdded(pqPipelineSource*)),
    this->Internal->YObject, SLOT(addSource(pqPipelineSource*)));
  QObject::connect(smmodel, SIGNAL(preSourceRemoved(pqPipelineSource*)),
    this->Internal->YObject, SLOT(removeSource(pqPipelineSource*)));

  QObject::connect(
    this->Internal->XObject, SIGNAL(currentIndexChanged(vtkSMProxy*)),
    this->Internal->XProperty, SLOT(setSource(vtkSMProxy*)));
  QObject::connect(
    this->Internal->YObject, SIGNAL(currentIndexChanged(vtkSMProxy*)),
    this->Internal->YProperty, SLOT(setSource(vtkSMProxy*)));

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
  model->setStartTime(0);
  model->setEndTime(1);

  if (mode == vtkSMComparativeViewProxy::FILM_STRIP)
    {
    pqAnimationTrack* track = model->addTrack();
    track->setProperty("Time Axis");
    pqAnimationKeyFrame* kf = track->addKeyFrame();
    kf->setStartTime(0);
    kf->setEndTime(1);
    kf->setStartValue(0);
    kf->setEndValue(1);

    this->Internal->XAxisGroup->setEnabled(true);
    this->Internal->XAxisGroup->setTitle("Animated Source");
    this->Internal->YAxisGroup->setVisible(false);
    }
  else
    {
    pqAnimationTrack* track = model->addTrack();
    track->setProperty("X Axis");
    pqAnimationKeyFrame* kf = track->addKeyFrame();
    kf->setStartTime(0);
    kf->setEndTime(1);
    kf->setStartValue(0);
    kf->setEndValue(1);

    track = model->addTrack();
    track->setProperty("Y Axis");
    kf = track->addKeyFrame();
    kf->setStartTime(0);
    kf->setEndTime(1);
    kf->setStartValue(0);
    kf->setEndValue(1);

    this->Internal->XAxisGroup->setEnabled(true);
    this->Internal->XAxisGroup->setTitle("Animated Source (X Axis)");
    this->Internal->YAxisGroup->setVisible(true);
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

//-----------------------------------------------------------------------------
void pqComparativeVisPanel::editPropertyToAnimate(int index)
{
  if (index <= 0)
    {
    return;
    }
  
}
