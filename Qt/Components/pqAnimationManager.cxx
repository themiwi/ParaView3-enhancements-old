/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqAnimationManager.cxx,v $

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
#include "pqAnimationManager.h"
#include "ui_pqAbortAnimation.h"
#include "ui_pqAnimationSettings.h"

#include "vtkSMAnimationSceneProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMServerProxyManagerReviver.h"

#include <QPointer>
#include <QMap>
#include <QtDebug>
#include <QFileInfo>

#include "pqAnimationCue.h"
#include "pqAnimationScene.h"
#include "pqApplicationCore.h"
#include "pqPipelineBuilder.h"
#include "pqRenderViewModule.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"

//-----------------------------------------------------------------------------
class pqAnimationManager::pqInternals
{
public:
  QPointer<pqServer> ActiveServer;
  typedef QMap<pqServer*, QPointer<pqAnimationScene> > SceneMap;
  SceneMap Scenes;
  Ui::Dialog* AnimationSettingsDialog;
};

//-----------------------------------------------------------------------------
pqAnimationManager::pqAnimationManager(QObject* _parent/*=0*/) 
:QObject(_parent)
{
  this->Internals = new pqAnimationManager::pqInternals();
  pqServerManagerModel* smmodel = 
    pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(smmodel, SIGNAL(proxyAdded(pqProxy*)),
    this, SLOT(onProxyAdded(pqProxy*)));
  QObject::connect(smmodel, SIGNAL(proxyRemoved(pqProxy*)),
    this, SLOT(onProxyRemoved(pqProxy*)));
}

//-----------------------------------------------------------------------------
pqAnimationManager::~pqAnimationManager()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onProxyAdded(pqProxy* proxy)
{
  pqAnimationScene* scene = qobject_cast<pqAnimationScene*>(proxy);
  if (scene && !this->Internals->Scenes.contains(scene->getServer()))
    {
    this->Internals->Scenes[scene->getServer()] = scene;
    if (this->Internals->ActiveServer == scene->getServer())
      {
      emit this->activeSceneChanged(this->getActiveScene());
      }
    }
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onProxyRemoved(pqProxy* proxy)
{
  pqAnimationScene* scene = qobject_cast<pqAnimationScene*>(proxy);
  if (scene) 
    {
    this->Internals->Scenes.remove(scene->getServer());
    if (this->Internals->ActiveServer == scene->getServer())
      {
      emit this->activeSceneChanged(this->getActiveScene());
      }
    }
}

//-----------------------------------------------------------------------------
void pqAnimationManager::onActiveServerChanged(pqServer* server)
{
  this->Internals->ActiveServer = server;
  emit this->activeSceneChanged(this->getActiveScene());
}

//-----------------------------------------------------------------------------
pqAnimationScene* pqAnimationManager::getActiveScene() const
{
  return this->getScene(this->Internals->ActiveServer);
}

//-----------------------------------------------------------------------------
pqAnimationScene* pqAnimationManager::getScene(pqServer* server) const
{
  if (server && this->Internals->Scenes.contains(server))
    {
    return this->Internals->Scenes.value(server);
    }
  return 0;
}

//-----------------------------------------------------------------------------
pqAnimationScene* pqAnimationManager::createActiveScene() 
{
  if (this->Internals->ActiveServer)
    {
    pqPipelineBuilder* builder = pqApplicationCore::instance()->getPipelineBuilder();
    vtkSMProxy *scene = builder->createProxy("animation", "AnimationScene", "animation",
      this->Internals->ActiveServer);
    // this will result in a call to onProxyAdded() and proper
    // signals will be emitted.
    if (!scene)
      {
      qDebug() << "Failed to create scene proxy.";
      }
    
    return this->getActiveScene();
    }
  return 0;
}


//-----------------------------------------------------------------------------
pqAnimationCue* pqAnimationManager::getCue(
  pqAnimationScene* scene, vtkSMProxy* proxy, const char* propertyname, 
  int index) const
{
  return (scene? scene->getCue(proxy, propertyname, index) : 0);
}


//-----------------------------------------------------------------------------
pqAnimationCue* pqAnimationManager::createCue(pqAnimationScene* scene, 
    vtkSMProxy* proxy, const char* propertyname, int index) 
{
  if (!scene)
    {
    qDebug() << "Cannot create cue without scene.";
    return 0;
    }

  return scene->createCue(proxy, propertyname, index);
}

//-----------------------------------------------------------------------------
void pqAnimationManager::durationChanged()
{
  double duration = 
    this->Internals->AnimationSettingsDialog->spinBoxAnimationDuration->value();
  double framerate =
    this->Internals->AnimationSettingsDialog->spinBoxFrameRate->value();

  double num_frames = duration*framerate;
  this->Internals->AnimationSettingsDialog->
    spinBoxNumberOfFrames->blockSignals(true);
  this->Internals->AnimationSettingsDialog->
    spinBoxNumberOfFrames->setValue(static_cast<int>(num_frames));
  this->Internals->AnimationSettingsDialog->
    spinBoxNumberOfFrames->blockSignals(false);
}

//-----------------------------------------------------------------------------
void pqAnimationManager::frameRateChanged()
{
  double framerate =
    this->Internals->AnimationSettingsDialog->spinBoxFrameRate->value();
  double num_frames = 
    this->Internals->AnimationSettingsDialog->spinBoxNumberOfFrames->value();

  double duration =  num_frames/framerate;

  this->Internals->AnimationSettingsDialog->
    spinBoxAnimationDuration->blockSignals(true);
  this->Internals->AnimationSettingsDialog->
    spinBoxAnimationDuration->setValue(static_cast<int>(duration));
  this->Internals->AnimationSettingsDialog->
    spinBoxAnimationDuration->blockSignals(false);
}

//-----------------------------------------------------------------------------
void pqAnimationManager::numberOfFramesChanged()
{
  this->frameRateChanged();
}

//-----------------------------------------------------------------------------
bool pqAnimationManager::saveAnimation(const QString& filename, 
  pqRenderViewModule* activeView)
{
  pqAnimationScene* scene = this->getActiveScene();
  if (!scene)
    {
    return false;
    }
  vtkSMAnimationSceneProxy* sceneProxy = scene->getAnimationSceneProxy();

  QFileInfo fileinfo(filename);
  QString filePrefix = filename;
  int dot_pos;
  if ((dot_pos = filename.lastIndexOf(".")) != -1)
    {
    filePrefix = filename.left(dot_pos);
    }
  QString extension = fileinfo.suffix();

  QDialog dialog;
  Ui::Dialog dialogUI;
  this->Internals->AnimationSettingsDialog = &dialogUI;
  dialogUI.setupUi(&dialog);
  dialogUI.checkBoxDisconnect->setEnabled(
    this->Internals->ActiveServer->isRemote());
  bool isMPEG = (extension == "mpg");
  if (isMPEG)
    {
    // Size bounds for mpeg.
    dialogUI.spinBoxWidth->setMaximum(1920);
    dialogUI.spinBoxHeight->setMaximum(1080);

    // Frame rate is fixed when using vtkMPIWriter.
    dialogUI.spinBoxFrameRate->setValue(30);
    dialogUI.spinBoxFrameRate->setEnabled(false);
    }
  else
    {
    // Size bounds for mpeg.
    dialogUI.spinBoxWidth->setMaximum(8000);
    dialogUI.spinBoxHeight->setMaximum(8000);
    }

  // Set current size of the window.
  QSize viewSize = activeView->getWidget()->size();
  dialogUI.spinBoxHeight->setValue(viewSize.height());
  dialogUI.spinBoxWidth->setValue(viewSize.width());

  // Set current duration/frame rate/no. of. frames.
  double start_time = pqSMAdaptor::getElementProperty(
    sceneProxy->GetProperty("StartTime")).toDouble();
  double end_time = pqSMAdaptor::getElementProperty(
    sceneProxy->GetProperty("EndTime")).toDouble();
  double duration = (end_time - start_time);
  double frame_rate = dialogUI.spinBoxFrameRate->value();
  double numFrames = duration*frame_rate;

  dialogUI.spinBoxNumberOfFrames->setValue(static_cast<int>(numFrames));
  dialogUI.spinBoxAnimationDuration->setValue(static_cast<int>(duration));

  QObject::connect(
    dialogUI.spinBoxAnimationDuration, SIGNAL(valueChanged(int)),
    this, SLOT(durationChanged()));
  QObject::connect(
    dialogUI.spinBoxFrameRate, SIGNAL(valueChanged(int)),
    this, SLOT(frameRateChanged()));
  QObject::connect(
    dialogUI.spinBoxNumberOfFrames, SIGNAL(valueChanged(int)),
    this, SLOT(numberOfFramesChanged()));

  if (!dialog.exec())
    {
    this->Internals->AnimationSettingsDialog = 0;
    return false;
    }
  this->Internals->AnimationSettingsDialog = 0;

  // Update Scene properties based on user options.
  pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("StartTime"), 0);
  pqSMAdaptor::setElementProperty(sceneProxy->GetProperty("EndTime"),
    dialogUI.spinBoxAnimationDuration->value());
  pqSMAdaptor::setProxyProperty(sceneProxy->GetProperty("RenderModule"),
    activeView->getProxy());
  sceneProxy->UpdateVTKObjects();

  QSize oldMaxSize = activeView->getWidget()->maximumSize();
  QSize newSize(dialogUI.spinBoxWidth->value(),
    dialogUI.spinBoxHeight->value());

  // Enfore the multiple of 4 criteria.
  if (isMPEG)
    {
    int &width = newSize.rwidth();
    int &height = newSize.rheight();
    if ((width % 32) > 0)
      {
      width -= width % 32;
      }
    if ((height % 8) > 0)
      {
      height -= height % 8;
      }
    if (width > 1920)
      {
      width = 1920;
      }
    if (height > 1080)
      {
      height = 1080;
      }
    }
  else
    {
    int &width = newSize.rwidth();
    int &height = newSize.rheight();
    if ((width % 4) > 0)
      {
      width -= width % 4;
      }
    if ((height % 4) > 0)
      {
      height -= height % 4;
      }
    }

  activeView->getWidget()->setMaximumSize(newSize);
  activeView->getWidget()->resize(newSize);
 

  if (dialogUI.checkBoxDisconnect->checkState() == Qt::Checked)
    {
    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    // We save the animation offline.
    vtkSMProxy* cleaner = 
      pxm->NewProxy("connection_cleaners", "AnimationPlayer");
    cleaner->SetConnectionID(this->Internals->ActiveServer->GetConnectionID());
    pxm->RegisterProxy("animation","cleaner",cleaner);
    cleaner->Delete();

    pqSMAdaptor::setElementProperty(cleaner->GetProperty("AnimationFileName"),
      filename.toAscii().data());
    pqSMAdaptor::setMultipleElementProperty(cleaner->GetProperty("Size"), 0,
      newSize.width());
    pqSMAdaptor::setMultipleElementProperty(cleaner->GetProperty("Size"), 1,
      newSize.height());
    pqSMAdaptor::setElementProperty(cleaner->GetProperty("FrameRate"),
      dialogUI.spinBoxFrameRate->value());
    cleaner->UpdateVTKObjects();

    vtkSMServerProxyManagerReviver* reviver = 
      vtkSMServerProxyManagerReviver::New();
    int status = reviver->ReviveRemoteServerManager(
        this->Internals->ActiveServer->GetConnectionID());
    reviver->Delete();
    pqApplicationCore::instance()->removeServer(this->Internals->ActiveServer);
    return status;
    }
 
  int status = sceneProxy->SaveImages(filePrefix.toAscii().data(),
    extension.toAscii().data(), newSize.width(), newSize.height(),
    dialogUI.spinBoxFrameRate->value(), 0);

  activeView->getWidget()->setMaximumSize(oldMaxSize);
  activeView->getWidget()->resize(viewSize);   
  return (status == 0);
}
