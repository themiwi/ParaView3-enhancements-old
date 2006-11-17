/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqThresholdPanel.cxx,v $

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

#include "pqThresholdPanel.h"

// Qt includes
#include <QDoubleSpinBox>
#include <QSlider>
#include <QComboBox>

// we include this for static plugins
#define QT_STATICPLUGIN
#include <QtPlugin>

// VTK includes

// ParaView Server Manager includes

// ParaView includes
#include "pqProxy.h"
#include "pqPropertyManager.h"
#include "pqFieldSelectionAdaptor.h"


QString pqThresholdPanelInterface::name() const
{
  return "Threshold";
}

pqObjectPanel* pqThresholdPanelInterface::createPanel(pqProxy* object_proxy, QWidget* p)
{
  return new pqThresholdPanel(object_proxy, p);
}

bool pqThresholdPanelInterface::canCreatePanel(pqProxy* proxy) const
{
  return (QString("Threshold") == proxy->getProxy()->GetXMLName()
     && QString("filters") == proxy->getProxy()->GetXMLGroup());
}

Q_EXPORT_PLUGIN(pqThresholdPanelInterface)


pqThresholdPanel::pqThresholdPanel(pqProxy* pxy, QWidget* p) :
  pqLoadedFormObjectPanel(":/pqWidgets/UI/pqThresholdPanel.ui", pxy, p)
{
  this->LowerSlider = this->findChild<QSlider*>("LowerThresholdSlider");
  this->UpperSlider = this->findChild<QSlider*>("UpperThresholdSlider");
  this->LowerSpin = this->findChild<QDoubleSpinBox*>("ThresholdBetween_Spin_0");
  this->UpperSpin = this->findChild<QDoubleSpinBox*>("ThresholdBetween_Spin_1");

  QObject::connect(this->LowerSlider, SIGNAL(valueChanged(int)),
                   this, SLOT(lowerSliderChanged()));
  QObject::connect(this->UpperSlider, SIGNAL(valueChanged(int)),
                   this, SLOT(upperSliderChanged()));
  QObject::connect(this->LowerSpin, SIGNAL(valueChanged(double)),
                   this, SLOT(lowerSpinChanged()));
  QObject::connect(this->UpperSpin, SIGNAL(valueChanged(double)),
                   this, SLOT(upperSpinChanged()));

  this->linkServerManagerProperties();
}

pqThresholdPanel::~pqThresholdPanel()
{
}

void pqThresholdPanel::accept()
{
  // accept widgets controlled by the parent class
  pqLoadedFormObjectPanel::accept();
}

void pqThresholdPanel::reset()
{
  // reset widgets controlled by the parent class
  pqLoadedFormObjectPanel::reset();
  
  // TODO: better linking of spin boxes and sliders,
  //       so we don't do this manually on reset
  this->upperSpinChanged();
  this->lowerSpinChanged();
}

void pqThresholdPanel::linkServerManagerProperties()
{
  // parent class hooks up some of our widgets in the ui
  pqLoadedFormObjectPanel::linkServerManagerProperties();
  
  QComboBox* combo = this->findChild<QComboBox*>("SelectInputScalars");
  vtkSMProperty* prop = this->proxy()->getProxy()->
          GetProperty("SelectInputScalars");
  pqFieldSelectionAdaptor* adaptor = new pqFieldSelectionAdaptor(combo, prop);

  this->propertyManager()->registerLink(adaptor, 
                                        "attributeMode",
                                        SIGNAL(selectionChanged()),
                                        this->proxy()->getProxy(),
                                        prop, 0);
  
  this->propertyManager()->registerLink(adaptor, 
                                        "scalar",
                                        SIGNAL(selectionChanged()),
                                        this->proxy()->getProxy(),
                                        prop, 1);

}

static bool IsSetting = false;

void pqThresholdPanel::lowerSpinChanged()
{
  if(IsSetting == false)
    {
    IsSetting = true;
    // spin changed, so update the slider
    // note: the slider is just a tag-along-guy to give a general indication of
    // range
    double v = this->LowerSpin->value();
    double range = this->LowerSpin->maximum() - this->LowerSpin->minimum();
    double fraction = (v - this->LowerSpin->minimum()) / range;
    int sliderVal = qRound(fraction * 100.0);  // slider range 0-100
    this->LowerSlider->setValue(sliderVal);
    IsSetting = false;
    
    // clamp the lower threshold if we need to
    if(this->UpperSpin->value() < this->LowerSpin->value())
      {
      this->UpperSpin->setValue(this->LowerSpin->value());
      }
    }
}

void pqThresholdPanel::upperSpinChanged()
{
  if(IsSetting == false)
    {
    IsSetting = true;
    // spin changed, so update the slider
    // note: the slider is just a tag-along-guy to give a general indication of
    // range
    double v = this->UpperSpin->value();
    double range = this->UpperSpin->maximum() - this->UpperSpin->minimum();
    double fraction = (v - this->UpperSpin->minimum()) / range;
    int sliderVal = qRound(fraction * 100.0);  // slider range 0-100
    this->UpperSlider->setValue(sliderVal);
    IsSetting = false;
    
    // clamp the lower threshold if we need to
    if(this->LowerSpin->value() > this->UpperSpin->value())
      {
      this->LowerSpin->setValue(this->UpperSpin->value());
      }
    }
}

void pqThresholdPanel::lowerSliderChanged()
{
  if(IsSetting == false)
    {
    IsSetting = true;
    double fraction = this->LowerSlider->value() / 100.0;
    double range = this->LowerSpin->maximum() - this->LowerSpin->minimum();
    double v = (fraction * range) + this->LowerSpin->minimum();
    this->LowerSpin->setValue(v);
    IsSetting = false;

    // clamp the upper threshold if we need to
    if(this->UpperSlider->value() < this->LowerSlider->value())
      {
      this->UpperSlider->setValue(this->LowerSlider->value());
      }
    }
}

void pqThresholdPanel::upperSliderChanged()
{
  if(IsSetting == false)
    {
    IsSetting = true;
    double fraction = this->UpperSlider->value() / 100.0;
    double range = this->UpperSpin->maximum() - this->UpperSpin->minimum();
    double v = (fraction * range) + this->UpperSpin->minimum();
    this->UpperSpin->setValue(v);
    IsSetting = false;
    
    // clamp the lower threshold if we need to
    if(this->LowerSlider->value() > this->UpperSlider->value())
      {
      this->LowerSlider->setValue(this->UpperSlider->value());
      }
    }
}

