/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqRenderViewBase.cxx,v $

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
#include "pqRenderViewBase.h"

// Server Manager Includes.
#include "QVTKWidget.h"
#include "vtkErrorCode.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkImageData.h"
#include "vtkProcessModule.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"

// Qt Includes.
#include <QList>
#include <QMenu>
#include <QMouseEvent>
#include <QPoint>
#include <QPointer>
#include <QtDebug>

// ParaView Includes.
#include "pqApplicationCore.h"
#include "pqImageUtil.h"
#include "pqOutputPort.h"
#include "pqServer.h"
#include "pqSettings.h"
#include "pqSMAdaptor.h"

class pqRenderViewBase::pqInternal
{
public:
  QPointer<QWidget> Viewport;
  QList<pqSMProxy> DefaultCameraManipulators;
  QPoint MouseOrigin;

  ~pqInternal()
    {
    delete this->Viewport;
    }
};

//-----------------------------------------------------------------------------
pqRenderViewBase::pqRenderViewBase(
  const QString& type,
  const QString& group,
  const QString& name, 
  vtkSMViewProxy* renViewProxy, 
  pqServer* server, 
  QObject* _parent/*=NULL*/):
  Superclass(type, group, name, renViewProxy, server, _parent)
{
  this->Internal = new pqRenderViewBase::pqInternal();
}

//-----------------------------------------------------------------------------
pqRenderViewBase::~pqRenderViewBase()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
QWidget* pqRenderViewBase::getWidget()
{
  if (!this->Internal->Viewport)
    {
    this->Internal->Viewport = this->createWidget();
    // we manage the context menu ourself, so it doesn't interfere with
    // render window interactions
    this->Internal->Viewport->setContextMenuPolicy(Qt::NoContextMenu);
    this->Internal->Viewport->installEventFilter(this);
    this->Internal->Viewport->setObjectName("Viewport");
    }

  return this->Internal->Viewport;
}

//-----------------------------------------------------------------------------
QWidget* pqRenderViewBase::createWidget() 
{
  QVTKWidget* vtkwidget = new QVTKWidget();

  // do image caching for performance
  // For now, we are doing this only on Apple because it can render
  // and capture a frame buffer even when it is obstructred by a
  // window. This does not work as well on other platforms.
#if defined(__APPLE__)
  vtkwidget->setAutomaticImageCacheEnabled(true);

  // help the QVTKWidget know when to clear the cache
  this->getConnector()->Connect(
    this->getProxy(), vtkCommand::ModifiedEvent,
    vtkwidget, SLOT(markCachedImageAsDirty()));
#endif

  return vtkwidget;
}

//-----------------------------------------------------------------------------
void pqRenderViewBase::initialize()
{
  this->Superclass::initialize();

  // The render module needs to obtain client side objects
  // for the RenderWindow etc. to initialize the QVTKWidget
  // correctly. It cannot do this unless the underlying proxy
  // has been created. Since any pqProxy should never call
  // UpdateVTKObjects() on itself in the constructor, we 
  // do the following.
  vtkSMProxy* proxy = this->getProxy();
  if (!proxy->GetObjectsCreated())
    {
    // Wait till first UpdateVTKObjects() call on the render module.
    // Under usual circumstances, after UpdateVTKObjects() the
    // render module objects will be created.
    this->getConnector()->Connect(proxy, vtkCommand::UpdateEvent,
      this, SLOT(initializeWidgets()));
    }
  else
    {
    this->initializeWidgets();
    }
}

//-----------------------------------------------------------------------------
// Sets default values for the underlying proxy.  This is during the 
// initialization stage of the pqProxy for proxies created by the GUI itself 
// i.e. for proxies loaded through state or created by python client or 
// undo/redo, this method won't be called. 
void pqRenderViewBase::setDefaultPropertyValues()
{
  this->createDefaultInteractors(this->Internal->DefaultCameraManipulators);
  this->setCameraManipulators(
    this->Internal->DefaultCameraManipulators);

  vtkSMProxy* proxy = this->getProxy();
  pqSMAdaptor::setElementProperty(proxy->GetProperty("LODResolution"), 50);
  pqSMAdaptor::setElementProperty(proxy->GetProperty("LODThreshold"), 5);
  pqSMAdaptor::setElementProperty(proxy->GetProperty("RemoteRenderThreshold"), 3);
  pqSMAdaptor::setElementProperty(proxy->GetProperty("SquirtLevel"), 3);

  const int* bg = this->defaultBackgroundColor();
  vtkSMProperty* backgroundProperty = proxy->GetProperty("Background");
  pqSMAdaptor::setMultipleElementProperty(backgroundProperty, 0, bg[0]/255.0);
  pqSMAdaptor::setMultipleElementProperty(backgroundProperty, 1, bg[1]/255.0);
  pqSMAdaptor::setMultipleElementProperty(backgroundProperty, 2, bg[2]/255.0);
  proxy->UpdateVTKObjects();

  this->restoreSettings(false);
  this->resetCamera();
}

//-----------------------------------------------------------------------------
// This method gets called only with the object is directly created by the GUI
// i.e. it wont get called when the proxy is loaded from state/undo/redo or 
// python.
// TODO: Python paraview modules createView() equivalent should make sure
// that it sets up some default interactor.
void pqRenderViewBase::createDefaultInteractors(QList<pqSMProxy>& manips)
{
  manips.clear();

  // subclass will give us the default manipulator types.
  const ManipulatorType* defaultManipTypes = 
    this->getDefaultManipulatorTypesInternal();
  for (int cc=0; cc < 9; cc++)
    {
    const ManipulatorType &manipType = defaultManipTypes[cc];
    vtkSMProxy *manip = this->createCameraManipulator(
      manipType.Mouse, manipType.Shift, manipType.Control, manipType.Name);
    manips.push_back(manip);
    manip->Delete();
    }
}

//-----------------------------------------------------------------------------
bool pqRenderViewBase::setCameraManipulators(const QList<pqSMProxy>& manipulators)
{
  if (manipulators.size()<=0)
    {
    return false;
    }

  vtkSMProxy* viewproxy = this->getProxy();
  
  this->clearHelperProxies();

  // Register manipulators, then add to interactor style
  foreach (vtkSMProxy *manip, manipulators)
    {
    this->addHelperProxy("Manipulators",manip);
    }

  pqSMAdaptor::setProxyListProperty(
    viewproxy->GetProperty("CameraManipulators"),
    manipulators);
  viewproxy->UpdateVTKObjects();

  return true;
}

//-----------------------------------------------------------------------------
QList<vtkSMProxy*> pqRenderViewBase::getCameraManipulators() const
{
  return this->getHelperProxies("Manipulators");
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqRenderViewBase::createCameraManipulator(
  int mouse, int shift, int control, QString name)
{
  QString strManipName;
  if(name.compare("Rotate")==0)
    {
    strManipName = "TrackballRotate";
    }
  else if(name.compare("Roll")==0)
    {
    strManipName = "TrackballRoll";
    }
  else if(name.compare("Move")==0)
    {
    strManipName = "TrackballMoveActor";
    }
  else if(name.compare("Zoom")==0)
    {
    strManipName = "TrackballZoom";
    }
  else if(name.compare("Pan")==0)
    {
    strManipName = "TrackballPan1";
    }
  else
    {
    strManipName = "None";
    }

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkIdType cid = this->getServer()->GetConnectionID();
  vtkSMProxy *manip = pxm->NewProxy("cameramanipulators", 
    strManipName.toAscii().data());
  if(!manip)
    {
    return NULL;
    }
  manip->SetConnectionID(cid);
  manip->SetServers(vtkProcessModule::CLIENT);
  pqSMAdaptor::setElementProperty(manip->GetProperty("Button"), mouse);
  pqSMAdaptor::setElementProperty(manip->GetProperty("Shift"), shift);
  pqSMAdaptor::setElementProperty(manip->GetProperty("Control"), control);
  pqSMAdaptor::setElementProperty(manip->GetProperty("ManipulatorName"), name);
  manip->UpdateVTKObjects();
  return manip;
}

//-----------------------------------------------------------------------------
const int* pqRenderViewBase::defaultBackgroundColor() const
{
  static int defaultBackground[3] = {0, 0, 0};
  return defaultBackground;
}

//-----------------------------------------------------------------------------
static const char* pqRenderViewModuleLightSettings [] = {
  "LightSwitch",
  "LightIntensity",
  "UseLight",
  "KeyLightWarmth",
  "KeyLightIntensity",
  "KeyLightElevation",
  "KeyLightAzimuth",
  "FillLightWarmth",
  "FillLightK:F Ratio",
  "FillLightElevation",
  "FillLightAzimuth",
  "BackLightWarmth",
  "BackLightK:B Ratio",
  "BackLightElevation",
  "BackLightAzimuth",
  "HeadLightWarmth",
  "HeadLightK:H Ratio",
  "MaintainLuminance",
  NULL
  };

static const char* pqGlobalRenderViewModuleMiscSettings [] = {
  "LODThreshold",
  "LODResolution",
  "UseImmediateMode",
  "UseTriangleStrips",
  "RenderInterruptsEnabled",
  "RemoteRenderThreshold",
  "ImageReductionFactor",
  "SquirtLevel",
  "OrderedCompositing",
  "StillRenderImageReductionFactor",
  "CollectGeometryThreshold",
  "DepthPeeling",
  "UseOffscreenRenderingForScreenshots",
  NULL
  };

static const char* pqRenderViewModuleMiscSettings [] = {
  "CacheLimit",
  "CameraParallelProjection",
  NULL
  };


static const char** pqRenderViewModuleSettings[] = {
  pqRenderViewModuleLightSettings,
  pqRenderViewModuleMiscSettings,
  NULL
  };

static const char** pqGlobalRenderViewModuleSettings[] = {
  pqGlobalRenderViewModuleMiscSettings,
  NULL
  };

static const char* pqRenderViewModuleLightSettingsMulti[] = {
  "LightDiffuseColor",
  NULL  // keep last
  };

static const char* pqRenderViewModuleMiscSettingsMulti[] = {
  "Background",
  NULL  // keep last
  };

static const char** pqRenderViewModuleSettingsMulti[] = {
  pqRenderViewModuleLightSettingsMulti,
  pqRenderViewModuleMiscSettingsMulti,
  NULL  // keep last
};

//-----------------------------------------------------------------------------
void pqRenderViewBase::restoreSettings(bool only_global)
{
  vtkSMProxy* proxy = this->getProxy();

  // Now load default values from the QSettings, if available.
  pqSettings* settings = pqApplicationCore::instance()->settings();
  
  const char*** str;
  if (!only_global)
    {
    settings->beginGroup(this->viewSettingsGroup());
    for (str=pqRenderViewModuleSettings; *str != NULL; str++)
      {
      const char** substr;
      for(substr = str[0]; *substr != NULL; substr++)
        {
        QString key = *substr;
        vtkSMProperty* prop = proxy->GetProperty(*substr);
        if (prop && settings->contains(key))
          {
          pqSMAdaptor::setElementProperty(prop, settings->value(key));
          }
        }
      }
    for (str=pqRenderViewModuleSettingsMulti; *str != NULL; str++)
      {
      const char** substr;
      for(substr = str[0]; *substr != NULL; substr++)
        {
        QString key = *substr;
        vtkSMProperty* prop = proxy->GetProperty(*substr);
        if (prop && settings->contains(key))
          {
          QList<QVariant> value = settings->value(key).value<QList<QVariant> >();
          pqSMAdaptor::setMultipleElementProperty(prop, value);
          }
        }
      }
    settings->endGroup();
    }
  
  settings->beginGroup(this->globalSettingsGroup());
  for (str=pqGlobalRenderViewModuleSettings; *str != NULL; str++)
    {
    const char** substr;
    for(substr = str[0]; *substr != NULL; substr++)
      {
      QString key = *substr;
      vtkSMProperty* prop = proxy->GetProperty(*substr);
      if (prop && settings->contains(key))
        {
        pqSMAdaptor::setElementProperty(prop, settings->value(key));
        }
      }
    }
  settings->endGroup();
  proxy->UpdateVTKObjects();

  settings->beginGroup(this->interactorStyleSettingsGroup());
  // Active Camera Manipulators
  if (settings->contains("CameraManipulators"))
    {
    QStringList qStrManipList = 
      settings->value("CameraManipulators").toStringList();
    int index, mouse, shift, control;
    QString name;
    char tmpName[20];
    QList<pqSMProxy> smManipList;
    foreach(QString strManip, qStrManipList)
      {
      sscanf(strManip.toAscii().data(), "Manipulator%dMouse%dShift%dControl%dName%s",
        &index, &mouse, &shift, &control, tmpName);
      name = tmpName;
      vtkSMProxy* localManip = this->createCameraManipulator(
        mouse, shift, control, name);
      if(!localManip)
        {
        continue;
        }
      smManipList.push_back(localManip);
      localManip->Delete();
      }
    this->setCameraManipulators(smManipList);
    }
  settings->endGroup();
}

//-----------------------------------------------------------------------------
void pqRenderViewBase::saveSettings()
{
  vtkSMProxy* proxy = this->getProxy();
  
  pqSettings* settings = pqApplicationCore::instance()->settings();

  settings->beginGroup(this->viewSettingsGroup());
  const char*** str;
  for(str=pqRenderViewModuleSettings; *str != NULL; str++)
    {
    const char** substr;
    for(substr = str[0]; *substr != NULL; substr++)
      {
      QString key = *substr;
      vtkSMProperty* prop = proxy->GetProperty(*substr);
      if (prop)
        {
        settings->setValue(key, pqSMAdaptor::getElementProperty(prop));
        }
      }
    }
  for(str=pqRenderViewModuleSettingsMulti; *str != NULL; str++)
    {
    const char** substr;
    for(substr = str[0]; *substr != NULL; substr++)
      {
      QString key = *substr;
      vtkSMProperty* prop = proxy->GetProperty(*substr);
      if (prop)
        {
        settings->setValue(key, pqSMAdaptor::getMultipleElementProperty(prop));
        }
      }
    }

  settings->endGroup();
}

//-----------------------------------------------------------------------------
void pqRenderViewBase::restoreDefaultLightSettings()
{
  vtkSMProxy* proxy = this->getProxy();
  const char** str;

  for (str=pqRenderViewModuleLightSettings; *str != NULL; str++)
    {
    vtkSMProperty* prop = proxy->GetProperty(*str);
    if(prop)
      {
      prop->ResetToDefault();
      }
    }
  for (str=pqRenderViewModuleLightSettingsMulti; *str != NULL; str++)
    {
    vtkSMProperty* prop = proxy->GetProperty(*str);
    prop->ResetToDefault();
    }
  proxy->UpdateVTKObjects();

}
  
//-----------------------------------------------------------------------------
bool pqRenderViewBase::eventFilter(QObject* caller, QEvent* e)
{
  if (e->type() == QEvent::MouseButtonPress)
    {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if (me->button() & Qt::RightButton)
      {
      this->Internal->MouseOrigin = me->pos();
      }
    }
  else if (e->type() == QEvent::MouseMove &&
    !this->Internal->MouseOrigin.isNull())
    {
    QPoint newPos = static_cast<QMouseEvent*>(e)->pos();
    QPoint delta = newPos - this->Internal->MouseOrigin;
    if(delta.manhattanLength() < 3)
      {
      this->Internal->MouseOrigin = QPoint();
      }
    }
  else if (e->type() == QEvent::MouseButtonRelease)
    {
    QMouseEvent* me = static_cast<QMouseEvent*>(e);
    if (me->button() & Qt::RightButton && !this->Internal->MouseOrigin.isNull())
      {
      QPoint newPos = static_cast<QMouseEvent*>(e)->pos();
      QPoint delta = newPos - this->Internal->MouseOrigin;
      if (delta.manhattanLength() < 3 && qobject_cast<QWidget*>(caller))
        {
        QList<QAction*> actions = this->Internal->Viewport->actions();
        if (!actions.isEmpty())
          {
          QMenu* menu = new QMenu(this->Internal->Viewport);
          menu->setAttribute(Qt::WA_DeleteOnClose);
          menu->addActions(actions);
          menu->popup(qobject_cast<QWidget*>(caller)->mapToGlobal(newPos));
          }
        }
      this->Internal->MouseOrigin = QPoint();
      }
    }
  
  return Superclass::eventFilter(caller, e);
}

//-----------------------------------------------------------------------------
bool pqRenderViewBase::canDisplay(pqOutputPort* opPort) const
{
  if (!opPort || 
     this->getServer()->GetConnectionID() != 
     opPort->getServer()->GetConnectionID())
    {
    return false;
    }
  return true;
}

//-----------------------------------------------------------------------------
bool pqRenderViewBase::saveImage(int width, int height, const QString& filename)
{
  QWidget* vtkwidget = this->getWidget();
  QSize cursize = vtkwidget->size();
  QSize fullsize = QSize(width, height);
  QSize newsize = cursize;
  int magnification = 1;
  if (width>0 && height>0)
    {
    magnification = pqView::computeMagnification(fullsize, newsize);
    vtkwidget->resize(newsize);
    }
  this->render();

  int error_code = vtkErrorCode::UnknownError;
  vtkImageData* vtkimage = this->captureImage(magnification);
  if (vtkimage)
    {
    error_code = pqImageUtil::saveImage(vtkimage, filename);
    vtkimage->Delete();
    }

  switch (error_code)
    {
  case vtkErrorCode::UnrecognizedFileTypeError:
    qCritical() << "Failed to determine file type for file:" 
      << filename.toAscii().data();
    break;

  case vtkErrorCode::NoError:
    // success.
    break;

  default:
    qCritical() << "Failed to save image.";
    }

  if (width>0 && height>0)
    {
    vtkwidget->resize(newsize);
    vtkwidget->resize(cursize);
    this->render();
    }
  return (error_code == vtkErrorCode::NoError);
}