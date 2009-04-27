/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqBarChartDisplayPanel.h,v $

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
#ifndef __pqBarChartDisplayPanel_h 
#define __pqBarChartDisplayPanel_h

#include "pqDisplayPanel.h"

class QModelIndex;

/// pqBarChartDisplayPanel is the display panel for BarChartRepresentation
/// proxy i.e. the representation for bar chart view.
class PQCOMPONENTS_EXPORT pqBarChartDisplayPanel : public pqDisplayPanel
{
  Q_OBJECT
  typedef pqDisplayPanel Superclass;
public:
  pqBarChartDisplayPanel(pqRepresentation* representation, QWidget* parent=0);
  ~pqBarChartDisplayPanel();

protected slots:
  /// Updates the state of the widgets showing the selected series options based
  /// on the active selection.
  void updateSeriesOptions();

  /// Update the series enabled state for currently selected series.
  void setCurrentSeriesEnabled(int state);

  /// Update the series color for currently selected series.
  void setCurrentSeriesColor(const QColor &color);

  /// Slot to listen to clicks for changing color.
  void activateItem(const QModelIndex &index);

private:
  Qt::CheckState getEnabledState() const;

private:
  pqBarChartDisplayPanel(const pqBarChartDisplayPanel&); // Not implemented.
  void operator=(const pqBarChartDisplayPanel&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};

#endif


