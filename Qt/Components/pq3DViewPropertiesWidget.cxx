/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pq3DViewPropertiesWidget.cxx,v $

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
#include "pq3DViewPropertiesWidget.h"
#include "ui_pq3DViewPropertiesWidget.h"

// ParaView Server Manager includes.
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSmartPointer.h"

// Qt includes.
#include <QDoubleValidator>
#include <QList>
#include <QComboBox>

// ParaView Client includes.
#include "pqApplicationCore.h"
#include "pqPropertyManager.h"
#include "pqRenderView.h"
#include "pqSettings.h"
#include "pqSignalAdaptors.h"
#include "pqSMAdaptor.h"
#include "pqNamedWidgets.h"

//-----------------------------------------------------------------------------
class pq3DViewPropertiesWidgetInternal : public Ui::pq3DViewProperties
{
public:
  pqPropertyManager Links;
  pqSignalAdaptorColor *ColorAdaptor;
  QList<QComboBox*> CameraControl3DComboBoxList;
  QList<QString> CameraControl3DComboItemList;
  QPointer<pqRenderView> ViewModule;

  pq3DViewPropertiesWidgetInternal() 
    {
    this->ColorAdaptor = 0;
    }

  ~pq3DViewPropertiesWidgetInternal()
    {
    delete this->ColorAdaptor;
    this->CameraControl3DComboBoxList.clear();
    this->CameraControl3DComboItemList.clear();
    }

  void updateLODThresholdLabel(int value)
    {
    this->lodThresholdLabel->setText(
      QString("%1").arg(value/10.0, 0, 'f', 2) + " MBytes");
    }

  void updateLODResolutionLabel(int value)
    {
    QVariant val(160-value + 10);

    this->lodResolutionLabel->setText(
      val.toString() + "x" + val.toString() + "x" + val.toString());
    }
  void updateOutlineThresholdLabel(int value)
    {
    this->outlineThresholdLabel->setText(
      QVariant(value/10.0).toString() + " MCells");
    }

  void updateCompositeThresholdLabel(int value)
    {
    this->compositeThresholdLabel->setText(
      QVariant(value/10.0).toString() + " MBytes");
    }
  void updateSubsamplingRateLabel(int value)
    {
    this->subsamplingRateLabel->setText(QVariant(value).toString() 
      + " Pixels");
    }
  void updateSquirtLevelLabel(int val)
    {
    static int bitValues[] = {24, 24, 22, 19, 16, 13, 10};
    val = (val < 0 )? 0 : val;
    val = ( val >6)? 6 : val;
    this->squirtLevelLabel->setText(
      QVariant(bitValues[val]).toString() + " Bits");
    }

  void updateStillSubsampleRateLabel(int value)
    {
    if (value == 1)
      {
      this->stillRenderSubsampleRateLabel->setText("Disabled");
      }
    else
      {
      this->stillRenderSubsampleRateLabel->setText(
        QString("%1 Pixels").arg(value));
      }
    }

  void updateClientCollectLabel(double value_in_mb)
    {
    this->clientCollectLabel->setText(
      QString("%1 MBytes").arg(value_in_mb));
    }

  void initializeGUICameraManipulators()
    {

    this->CameraControl3DComboBoxList << this->comboBoxCamera3D
    << this->comboBoxCamera3D_2 << this->comboBoxCamera3D_3
    << this->comboBoxCamera3D_4 << this->comboBoxCamera3D_5
    << this->comboBoxCamera3D_6 << this->comboBoxCamera3D_7
    << this->comboBoxCamera3D_8 << this->comboBoxCamera3D_9;
  
    this->CameraControl3DComboItemList //<< "FlyIn" << "FlyOut" << "Move"
       << "Pan" << "Roll" << "Rotate" << "Zoom";

    vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
    for ( int cc = 0; cc < this->CameraControl3DComboBoxList.size(); cc++ )
      {
      foreach(QString name, this->CameraControl3DComboItemList)
        {
        this->CameraControl3DComboBoxList.at(cc)->addItem(name);
        }
      }
    }

  bool supportsCompositing(vtkSMProxy* proxy)
    {
    return (proxy && proxy->GetProperty("RemoteRenderThreshold"));
    }

  bool supportsClientServer(vtkSMProxy* proxy)
    {
    return (proxy && proxy->GetProperty("ImageReductionFactor") &&
      proxy->GetProperty("SquirtLevel"));
    }

  void loadValues(pqRenderView* proxy);
  void accept();
  void setGUICameraManipulators(QList<vtkSMProxy*> manipulators);
  void updateCameraManipulators(pqRenderView* rm);
  void resetCameraManipulators();
};

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidgetInternal::loadValues(pqRenderView* viewModule)
{
  if (!this->ColorAdaptor)
    {
    this->ColorAdaptor = new pqSignalAdaptorColor(this->backgroundColor, 
      "chosenColor", SIGNAL(chosenColorChanged(const QColor&)), false);
    }

  this->ViewModule = viewModule;
  vtkSMProxy* proxy = viewModule->getProxy();

  this->Links.registerLink(this->ColorAdaptor, "color",
    SIGNAL(colorChanged(const QVariant&)),
    proxy, proxy->GetProperty("Background"));

  this->Links.registerLink(this->cacheLimit, "value", SIGNAL(valueChanged(int)),
    proxy, proxy->GetProperty("CacheLimit"));
  this->Links.registerLink(this->parallelProjection, "checked",
    SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("CameraParallelProjection"));

  this->Links.registerLink(this->triangleStrips, "checked", 
    SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("UseTriangleStrips"));

  this->Links.registerLink(this->immediateModeRendering, "checked", 
    SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("UseImmediateMode"));

  // link default light params
  this->Links.registerLink(this->DefaultLightSwitch, "checked", 
    SIGNAL(toggled(bool)),
    proxy, proxy->GetProperty("LightSwitch"));
  pqSignalAdaptorSliderRange* sliderAdaptor;
  sliderAdaptor = new pqSignalAdaptorSliderRange(this->LightIntensity);
  this->Links.registerLink(sliderAdaptor, "value",
    SIGNAL(valueChanged(double)),
    proxy, proxy->GetProperty("LightIntensity"));
  this->Links.registerLink(this->LightIntensity_Edit, "text",
    SIGNAL(textChanged(const QString&)),
    proxy, proxy->GetProperty("LightIntensity"));
  pqSignalAdaptorColor* lightColorAdaptor;
  lightColorAdaptor = new pqSignalAdaptorColor(this->SetLightColor,
    "chosenColor",
    SIGNAL(chosenColorChanged(const QColor&)),
    false);
  this->Links.registerLink(lightColorAdaptor, "color",
    SIGNAL(colorChanged(const QVariant&)),
    proxy, proxy->GetProperty("LightDiffuseColor"));


  // link light kit params
  pqNamedWidgets::link(this->UseLight, proxy, &this->Links);
  this->Links.registerLink(this->UseLight, "checked", SIGNAL(toggled(bool)),
    proxy, proxy->GetProperty("UseLight"));


  this->lodParameters->setVisible(true);
  double val = pqSMAdaptor::getElementProperty(
    proxy->GetProperty("LODThreshold")).toDouble();
  if (val >= VTK_LARGE_FLOAT)
    {
    this->enableLOD->setCheckState(Qt::Unchecked);
    this->updateLODThresholdLabel(this->lodThreshold->value());
    }
  else
    {
    this->enableLOD->setCheckState(Qt::Checked);
    this->lodThreshold->setValue(static_cast<int>(val*10));
    this->updateLODThresholdLabel(this->lodThreshold->value());
    }

  val = pqSMAdaptor::getElementProperty(
    proxy->GetProperty("LODResolution")).toDouble();
  this->lodResolution->setValue(static_cast<int>(160-val + 10));
  this->updateLODResolutionLabel(this->lodResolution->value());

  this->Links.registerLink(this->renderingInterrupts, "checked",
    SIGNAL(stateChanged(int)),
    proxy, proxy->GetProperty("RenderInterruptsEnabled"));

  this->noServerSettingsLabel->setVisible(true);
  if (this->supportsCompositing(proxy))
    {
    this->noServerSettingsLabel->setVisible(false);
    this->compositingParameters->setVisible(true);
    val = pqSMAdaptor::getElementProperty(
      proxy->GetProperty("RemoteRenderThreshold")).toDouble();
    if (val >= VTK_LARGE_FLOAT)
      {
      this->enableCompositing->setCheckState(Qt::Unchecked);
      this->updateCompositeThresholdLabel(this->compositeThreshold->value());
      }
    else
      {
      this->enableCompositing->setCheckState(Qt::Checked);
      this->compositeThreshold->setValue(static_cast<int>(val*10));
      this->updateCompositeThresholdLabel(this->compositeThreshold->value());
      }

    if (proxy->GetProperty("DisableOrderedCompositing"))
      {
      this->orderedCompositing->setVisible(true);
      this->Links.registerLink(this->orderedCompositing, "checked", 
        SIGNAL(stateChanged(int)),
        proxy, proxy->GetProperty("DisableOrderedCompositing"));
      }
    else
      {
      this->orderedCompositing->setVisible(false);
      }
    }
  else
    {
    this->compositingParameters->setVisible(false);
    }

  if (this->supportsClientServer(proxy))
    {
    this->clientServerParameters->setVisible(true);
    int ival = pqSMAdaptor::getElementProperty(
      proxy->GetProperty("ImageReductionFactor")).toInt();
    if (ival == 1)
      {
      this->enableSubsampling->setCheckState(Qt::Unchecked);
      this->updateSubsamplingRateLabel(this->subsamplingRate->value());
      }
    else
      {
      this->enableSubsampling->setCheckState(Qt::Checked);
      this->subsamplingRate->setValue(ival);
      this->updateSubsamplingRateLabel(this->subsamplingRate->value());
      }

    ival = pqSMAdaptor::getElementProperty(
      proxy->GetProperty("SquirtLevel")).toInt();
    if (ival == 0)
      {
      this->enableSquirt->setCheckState(Qt::Unchecked);
      this->updateSquirtLevelLabel(this->squirtLevel->value());
      }
    else
      {
      this->enableSquirt->setCheckState(Qt::Checked);
      this->squirtLevel->setValue(ival);
      this->updateSquirtLevelLabel(this->squirtLevel->value());
      }
    }
  else
    {
    this->clientServerParameters->setVisible(false);
    }


  if (proxy->IsA("vtkSMIceTRenderModuleProxy"))
    {
    // Only for tiledisplay render modules.
    this->noServerSettingsLabel->setVisible(false);
    this->tileDisplayParameters->setVisible(true);
    int ival = pqSMAdaptor::getElementProperty(
      proxy->GetProperty("StillReductionFactor")).toInt();
    if (ival == 1)
      {
      this->enableStillRenderSubsampleRate->setCheckState(Qt::Unchecked);
      this->updateStillSubsampleRateLabel(
        this->stillRenderSubsampleRate->value());
      }
    else
      {
      this->enableStillRenderSubsampleRate->setCheckState(Qt::Checked);
      this->stillRenderSubsampleRate->setValue(ival);
      this->updateStillSubsampleRateLabel(
        this->stillRenderSubsampleRate->value());
      }

    double dval = pqSMAdaptor::getElementProperty(
      proxy->GetProperty("CollectGeometryThreshold")).toDouble();
    if (dval >= VTK_LARGE_FLOAT)
      {
      this->enableClientCollect->setCheckState(Qt::Unchecked);
      this->updateClientCollectLabel(this->clientCollect->value());
      }
    else
      {
      this->enableClientCollect->setCheckState(Qt::Checked);
      this->clientCollect->setValue(static_cast<int>(dval));
      this->updateClientCollectLabel(this->clientCollect->value());
      }
    }
  else
    {
    this->tileDisplayParameters->setVisible(false);
    }

  this->OrientationAxes->setChecked(this->ViewModule->getOrientationAxesVisibility());
  this->OrientationAxesInteraction->setCheckState(
    this->ViewModule->getOrientationAxesInteractivity()? Qt::Checked : Qt::Unchecked);
  this->OrientationAxesOutlineColor->setChosenColor(
    this->ViewModule->getOrientationAxesOutlineColor());
  this->OrientationAxesLabelColor->setChosenColor(
    this->ViewModule->getOrientationAxesLabelColor());

  this->CustomCenter->setCheckState(Qt::Unchecked);
  this->AutoResetCenterOfRotation->setCheckState(
    this->ViewModule->getResetCenterWithCamera()? Qt::Checked : Qt::Unchecked);
  this->CenterAxesVisibility->setCheckState(
    this->ViewModule->getCenterAxesVisibility()? Qt::Checked : Qt::Unchecked);
  double center[3];
  this->ViewModule->getCenterOfRotation(center);
  this->CenterX->setText(QString::number(center[0],'g',3));
  this->CenterY->setText(QString::number(center[1],'g',3));
  this->CenterZ->setText(QString::number(center[2],'g',3));

  if(this->ViewModule)
    {
    this->setGUICameraManipulators(this->ViewModule
      ->getCameraManipulators());
    }
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidgetInternal::setGUICameraManipulators(
  QList<vtkSMProxy*> manipulators)
{
  if(manipulators.size()<=0)
    {
    return;
    }

  pqRenderView* rm = this->ViewModule;
  int mouse, key, shift, control, pos, index;
  QString name;
if(rm->getCameraManipulators().size()>0)
  {
  foreach(vtkSMProxy* manip, manipulators)
    {
    key = 0;
    mouse = pqSMAdaptor::getElementProperty(
      manip->GetProperty("Button")).toInt();
    shift = pqSMAdaptor::getElementProperty(
      manip->GetProperty("Shift")).toInt();
    control = pqSMAdaptor::getElementProperty(
      manip->GetProperty("Control")).toInt();
    name = pqSMAdaptor::getElementProperty(
      manip->GetProperty("ManipulatorName")).toString();
    
    if(!this->CameraControl3DComboItemList.contains(name))
      {
      continue;
      }

    key = (shift==1) ? 1 : 0;
    if(!key)
      {
      key = (control==1) ? 2 : 0;
      }
    pos = mouse + key*3;
    if(pos<1 || pos > this->CameraControl3DComboBoxList.size())
      {
      continue;
      }
    index = this->CameraControl3DComboItemList.indexOf(name);
    this->CameraControl3DComboBoxList[pos-1]->setCurrentIndex(index);
    }
  }
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidgetInternal::accept()
{
  if(!this->ViewModule)
    {
    return;
    }

  // We need to accept user changes.
  this->Links.accept();

  vtkSMProxy* renModule = this->ViewModule->getProxy();

  // Push changes for LOD parameters.
  if (this->enableLOD->checkState() == Qt::Checked)
    {
    pqSMAdaptor::setElementProperty(
      renModule->GetProperty("LODThreshold"), 
      this->lodThreshold->value() / 10.0);

    pqSMAdaptor::setElementProperty(
      renModule->GetProperty("LODResolution"),
      160-this->lodResolution->value() + 10);
    }
  else
    {
    pqSMAdaptor::setElementProperty(
      renModule->GetProperty("LODThreshold"), VTK_DOUBLE_MAX);
    }

  if (this->supportsCompositing(renModule))
    {
    if (this->enableCompositing->checkState() == Qt::Checked)
      {
      pqSMAdaptor::setElementProperty(
        renModule->GetProperty("RemoteRenderThreshold"),
        this->compositeThreshold->value() / 10.0);
      }
    else
      {
      pqSMAdaptor::setElementProperty(
        renModule->GetProperty("RemoteRenderThreshold"), VTK_DOUBLE_MAX);
      }

    if (renModule->GetProperty("DisableOrderedCompositing"))
      {
      pqSMAdaptor::setElementProperty(
        renModule->GetProperty("DisableOrderedCompositing"),
        (this->orderedCompositing->checkState() == Qt::Checked));
      }
    }

  if (this->supportsClientServer(renModule))
    {
    if (this->enableSubsampling->checkState() == Qt::Checked)
      {
      pqSMAdaptor::setElementProperty(
        renModule->GetProperty("ImageReductionFactor"),
        this->subsamplingRate->value());
      }
    else
      {
      pqSMAdaptor::setElementProperty(
        renModule->GetProperty("ImageReductionFactor"), 1);
      }

    if (this->enableSquirt->checkState() == Qt::Checked)
      {
      pqSMAdaptor::setElementProperty(
        renModule->GetProperty("SquirtLevel"),
        this->squirtLevel->value());
      }
    else
      {
      pqSMAdaptor::setElementProperty(
        renModule->GetProperty("SquirtLevel"), 0);
      }
    }

  if (renModule->IsA("vtkSMIceTRenderModuleProxy"))
    {
    if (this->enableStillRenderSubsampleRate->checkState() == Qt::Checked)
      {
      pqSMAdaptor::setElementProperty(
        renModule->GetProperty("StillReductionFactor"),
        this->stillRenderSubsampleRate->value());
      }
    else
      {
      pqSMAdaptor::setElementProperty(
        renModule->GetProperty("StillReductionFactor"), VTK_DOUBLE_MAX);
      }

    if (this->enableClientCollect->checkState() == Qt::Checked)
      {
      pqSMAdaptor::setElementProperty(
        renModule->GetProperty("CollectGeometryThreshold"),
        this->clientCollect->value());
      }
    else
      {
      pqSMAdaptor::setElementProperty(
        renModule->GetProperty("CollectGeometryThreshold"), VTK_DOUBLE_MAX);
      }
    }
  renModule->UpdateVTKObjects();

  this->ViewModule->setOrientationAxesVisibility(this->OrientationAxes->isChecked());

  this->ViewModule->setOrientationAxesInteractivity(
    this->OrientationAxesInteraction->checkState() == Qt::Checked);
  this->ViewModule->setOrientationAxesOutlineColor(
    this->OrientationAxesOutlineColor->chosenColor());
  this->ViewModule->setOrientationAxesLabelColor(
    this->OrientationAxesLabelColor->chosenColor());

  this->ViewModule->setCenterAxesVisibility(
    this->CenterAxesVisibility->checkState() == Qt::Checked);
  this->ViewModule->setResetCenterWithCamera(
    this->AutoResetCenterOfRotation->checkState() == Qt::Checked);
  if (this->CustomCenter->checkState() == Qt::Checked)
    {
    double center[3];
    center[0] = this->CenterX->text().toDouble();
    center[1] = this->CenterY->text().toDouble();
    center[2] = this->CenterZ->text().toDouble();
    this->ViewModule->setCenterOfRotation(center);
    }
  this->updateCameraManipulators(this->ViewModule);
  this->ViewModule->saveSettings();
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidgetInternal::updateCameraManipulators(
  pqRenderView* rm)
{
  int mouse, key, shift, control;
  QString name;
  QList<vtkSMProxy*> smManipList;
  for ( int cc = 0; cc < this->CameraControl3DComboBoxList.size(); cc++ )
    {
    shift = 0;
    control = 0;
    mouse = cc % 3; //"mouse button" can only be 1, 2, 3.
    key = static_cast<int>(cc / 3);
    shift = (key == 1);
    control = (key == 2); 
    name = this->CameraControl3DComboBoxList[cc]->currentText();
    vtkSMProxy* localManip = rm->createCameraManipulator(
      mouse+1, shift, control, name);
    smManipList.push_back(localManip);
    }

  if(smManipList.size()>0)
    {
    rm->updateDefaultInteractors(smManipList);
    foreach(vtkSMProxy* localManip, smManipList)
      {
      localManip->Delete();
      }
    smManipList.clear();
    }
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidgetInternal::resetCameraManipulators()
{
  if(this->ViewModule)
    {
    this->setGUICameraManipulators(
      this->ViewModule->getDefaultCameraManipulators());
    }

}

//-----------------------------------------------------------------------------
pq3DViewPropertiesWidget::pq3DViewPropertiesWidget(QWidget* _parent):
  QWidget(_parent)
{
  this->Internal = new pq3DViewPropertiesWidgetInternal;
  this->Internal->setupUi(this);
  this->Internal->initializeGUICameraManipulators();

  QDoubleValidator* dv = new QDoubleValidator(this);
  this->Internal->CenterX->setValidator(dv);
  this->Internal->CenterY->setValidator(dv);
  this->Internal->CenterZ->setValidator(dv);

  this->Internal->label_3->hide();
  this->Internal->outlineThresholdLabel->hide();
  this->Internal->outlineThreshold->hide();
  QObject::connect(this->Internal->lodThreshold,
    SIGNAL(valueChanged(int)), this, SLOT(lodThresholdSliderChanged(int)));

  QObject::connect(this->Internal->lodResolution,
    SIGNAL(valueChanged(int)), this, SLOT(lodResolutionSliderChanged(int)));

  QObject::connect(this->Internal->outlineThreshold,
    SIGNAL(valueChanged(int)), this, SLOT(outlineThresholdSliderChanged(int)));

  QObject::connect(this->Internal->compositeThreshold,
    SIGNAL(valueChanged(int)), this, SLOT(compositeThresholdSliderChanged(int)));

  QObject::connect(this->Internal->subsamplingRate,
    SIGNAL(valueChanged(int)), this, SLOT(subsamplingRateSliderChanged(int)));

  QObject::connect(this->Internal->squirtLevel,
    SIGNAL(valueChanged(int)), this, SLOT(squirtLevelRateSliderChanged(int)));
  
  QObject::connect(this->Internal->restoreDefault,
    SIGNAL(clicked(bool)), this, SLOT(restoreDefaultBackground()));
  
  QObject::connect(this->Internal->ResetLight,
    SIGNAL(clicked(bool)), this, SLOT(resetLights()));

  QObject::connect(this->Internal->stillRenderSubsampleRate, 
    SIGNAL(valueChanged(int)), 
    this, SLOT(stillRenderSubsampleRateSliderChanged(int)));
  
  QObject::connect(this->Internal->clientCollect,
    SIGNAL(valueChanged(int)),
    this, SLOT(clientCollectSliderChanged(int)));

  QObject::connect(this->Internal->resetCameraDefault,
    SIGNAL(clicked()), this, SLOT(resetDefaultCameraManipulators()));
  
}

//-----------------------------------------------------------------------------
pq3DViewPropertiesWidget::~pq3DViewPropertiesWidget()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::setRenderView(pqRenderView* renderView)
{
  this->Internal->loadValues(renderView);
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::accept()
{
  this->Internal->accept();
  if(this->Internal->ViewModule)
    {
    this->Internal->ViewModule->render();
    }
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::lodThresholdSliderChanged(int value)
{
  this->Internal->updateLODThresholdLabel(value);
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::lodResolutionSliderChanged(int value)
{
  this->Internal->updateLODResolutionLabel(value);
}
//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::outlineThresholdSliderChanged(int value)
{
  this->Internal->updateOutlineThresholdLabel(value);
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::compositeThresholdSliderChanged(int value)
{
  this->Internal->updateCompositeThresholdLabel(value);
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::subsamplingRateSliderChanged(int value)
{
  this->Internal->updateSubsamplingRateLabel(value);
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::squirtLevelRateSliderChanged(int value)
{
  this->Internal->updateSquirtLevelLabel(value);
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::stillRenderSubsampleRateSliderChanged(int value)
{
  this->Internal->updateStillSubsampleRateLabel(value);
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::clientCollectSliderChanged(int value)
{
  this->Internal->updateClientCollectLabel(static_cast<double>(value));
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::resetDefaultCameraManipulators()
{
  this->Internal->resetCameraManipulators();
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::restoreDefaultBackground()
{
  if (this->Internal->ViewModule)
    {
    int* col = this->Internal->ViewModule->defaultBackgroundColor();
    this->Internal->backgroundColor->setChosenColor(
               QColor(col[0], col[1], col[2]));
    }
}

//-----------------------------------------------------------------------------
void pq3DViewPropertiesWidget::resetLights()
{
  if(this->Internal->ViewModule)
    {
    this->Internal->ViewModule->restoreDefaultLightSettings();
    }
}


