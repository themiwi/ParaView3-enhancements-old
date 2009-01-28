/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqBarChartView.h,v $

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

========================================================================*/
#ifndef __pqBarChartView_h 
#define __pqBarChartView_h

#include "pqView.h"

class vtkSMSourceProxy;
class pqDataRepresentation;

/// Bar chart view
class PQCORE_EXPORT pqBarChartView : public pqView
{
  Q_OBJECT
  typedef pqView Superclass;
public:
  static QString barChartViewType() { return "BarChartView2"; }
  static QString barChartViewTypeName() { return "Bar Chart View 2"; }

public:
  pqBarChartView(const QString& group, const QString& name, 
    vtkSMViewProxy* viewModule, pqServer* server, 
    QObject* parent=NULL);
  virtual ~pqBarChartView();

  /// Return a widget associated with this view.
  /// This view has no widget.
  virtual QWidget* getWidget();

  virtual void setDefaultPropertyValues();

  /// Requests a delayed chart widget render.
  virtual void render();

  /// This view does not support saving to image.
  virtual bool saveImage(int /*width*/, int /*height*/, 
    const QString& /*filename*/)
    { return false; }

  /// This view does not support image capture, return 0;
  virtual vtkImageData* captureImage(int /*magnification*/)
    { return 0; }
  virtual vtkImageData* captureImage(const QSize& asize)
    { return this->Superclass::captureImage(asize); } 

  virtual bool supportsUndo() const {return true;}

  /// Called to undo interaction.
  virtual void undo();

  /// Called to redo interaction.
  virtual void redo();

  /// Returns true if undo can be done.
  virtual bool canUndo() const;

  /// Returns true if redo can be done.
  virtual bool canRedo() const;

  /// 
  virtual bool canDisplay(pqOutputPort* opPort) const;

protected slots:
  /// Called when a new repr is added.
  void onAddRepresentation(pqRepresentation*);
  void onRemoveRepresentation(pqRepresentation*);

  /// Called to ensure that at most 1 repr is visible at a time.
  void updateRepresentationVisibility(pqRepresentation* repr, bool visible);

  void updateTitle();
  void updateTitleFont();
  void updateTitleColor();
  void updateTitleAlignment();

  void updateAxisTitle();
  void updateAxisTitleFont();
  void updateAxisTitleColor();
  void updateAxisTitleAlignment();

  void updateLegendVisibility();
  void updateLegendLocation();
  void updateLegendFlow();

  void updateAxisVisibility();
  void updateAxisColor();

  void updateGridVisibility();
  void updateGridColorType();
  void updateGridColor();

  void updateAxisLabelVisibility();
  void updateAxisLabelFont();
  void updateAxisLabelColor();
  void updateAxisLabelPrecision();
  void updateAxisLabelNotation();

  void updateAxisScale();
  void updateAxisBehavior();
  void updateAxisRange();
  void updateLeftAxisLabels();
  void updateBottomAxisLabels();
  void updateRightAxisLabels();
  void updateTopAxisLabels();
  
private:
  pqBarChartView(const pqBarChartView&); // Not implemented.
  void operator=(const pqBarChartView&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};

#endif
