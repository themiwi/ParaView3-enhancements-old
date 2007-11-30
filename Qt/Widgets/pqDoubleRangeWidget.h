/*=========================================================================

   Program:   ParaView
   Module:    $RCSfile: pqDoubleRangeWidget.h,v $

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
#ifndef __pqDoubleRangeWidget_h
#define __pqDoubleRangeWidget_h

#include <QWidget>
#include "QtWidgetsExport.h"
  
class QSlider;
class QLineEdit;

/// a widget with a tied slider and line edit for editing a double property
class QTWIDGETS_EXPORT pqDoubleRangeWidget : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(double value READ value WRITE setValue USER true)
  Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
  Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
  Q_PROPERTY(bool   strictRange READ strictRange WRITE setStrictRange)
public:
  /// constructor requires the proxy, property
  pqDoubleRangeWidget(QWidget* parent = NULL);
  ~pqDoubleRangeWidget();

  /// get the value
  double value() const;
  
  // get the min range value
  double minimum() const;
  // get the max range value
  double maximum() const;
 
  // returns whether the line edit is also limited 
  bool strictRange() const;
  
signals:
  /// signal the value changed
  void valueChanged(double);

public slots:
  /// set the value
  void setValue(double);

  // set the min range value
  void setMinimum(double);
  // set the max range value
  void setMaximum(double);

  // set the range on both the slider and line edit's validator
  // whereas other methods just do it on the slider
  void setStrictRange(bool);

private slots:
  void sliderChanged(int);
  void textChanged(const QString&);
  void updateValidator();
  void updateSlider();

private:
  double Value;
  double Minimum;
  double Maximum;
  QSlider* Slider;
  QLineEdit* LineEdit;
  bool BlockUpdate;
  bool StrictRange;
};

#endif

