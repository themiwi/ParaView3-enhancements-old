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

#include "pqChartView.h"

class vtkSMSourceProxy;
class pqDataRepresentation;
class vtkQtBarChartView;

/// Bar chart view
class PQCORE_EXPORT pqBarChartView : public pqChartView
{
  Q_OBJECT
  typedef pqChartView Superclass;

public:
  static QString barChartViewType() { return "BarChartView"; }
  static QString barChartViewTypeName() { return "Bar Chart View"; }

public:
  pqBarChartView(const QString& group,
                 const QString& name, 
                 vtkSMChartViewProxy* viewModule,
                 pqServer* server, 
                 QObject* parent=NULL);

  virtual ~pqBarChartView();

  /// Set property values.
  virtual void setDefaultPropertyValues();

private:
  pqBarChartView(const pqBarChartView&); // Not implemented.
  void operator=(const pqBarChartView&); // Not implemented.
};

#endif
