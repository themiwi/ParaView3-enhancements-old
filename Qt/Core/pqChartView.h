/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqChartView.h,v $

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
#ifndef __pqChartView_h 
#define __pqChartView_h

#include "pqView.h"

class vtkSMChartViewProxy;
class vtkQtChartView;

/// pqChartView is an abstract base class for all charting views based on VTK
/// Charting library.
class PQCORE_EXPORT pqChartView : public pqView
{
  Q_OBJECT
  typedef pqView Superclass;
public:
  virtual ~pqChartView();

  /// Return a widget associated with this view.
  virtual QWidget* getWidget();

  /// Returns the internal vtkQtChartView which provide the implementation for
  /// the chart rendering.
  vtkQtChartView* getVTKChartView() const;

  /// This view does not support saving to image.
  virtual bool saveImage(int width, int height, const QString& filename);

  /// Capture the view image into a new vtkImageData with the given magnification
  /// and returns it. The caller is responsible for freeing the returned image.
  virtual vtkImageData* captureImage(int magnification);

  /// Capture the view image of the given size and returns it. The caller is
  /// responsible for freeing the returned image.
  virtual vtkImageData* captureImage(const QSize& asize);

  /// This view supports undo so this method always returns true.
  virtual bool supportsUndo() const {return true;}

  /// Called to undo interaction.
  virtual void undo();

  /// Called to redo interaction.
  virtual void redo();

  /// Returns true if undo can be done.
  virtual bool canUndo() const;

  /// Returns true if redo can be done.
  virtual bool canRedo() const;

  /// Resets the zoom level to 100%.
  virtual void resetDisplay();

  /// Returns true if data on the given output port can be displayed by this view.
  virtual bool canDisplay(pqOutputPort* opPort) const;

protected:
  /// Constructor:
  /// \c type  :- view type.
  /// \c group :- SManager registration group.
  /// \c name  :- SManager registration name.
  /// \c view  :- View proxy.
  /// \c server:- server on which the proxy is created.
  /// \c parent:- QObject parent.
  pqChartView(const QString& type,
    const QString& group, 
    const QString& name, 
    vtkSMChartViewProxy* view, 
    pqServer* server, 
    QObject* parent=NULL);

private:
  pqChartView(const pqChartView&); // Not implemented.
  void operator=(const pqChartView&); // Not implemented.
};

#endif


