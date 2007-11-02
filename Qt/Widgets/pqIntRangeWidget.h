/*=========================================================================

   Program:   ParaView
   Module:    $RCSfile: pqIntRangeWidget.h,v $

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
#ifndef __pqIntRangeWidget_h
#define __pqIntRangeWidget_h

#include <QWidget>
#include "QtWidgetsExport.h"
  
class QSlider;
class QLineEdit;

/// a widget with a tied slider and line edit for editing a int property
class QTWIDGETS_EXPORT pqIntRangeWidget : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(int value READ value WRITE setValue)
public:
  /// constructor requires the proxy, property
  pqIntRangeWidget(QWidget* parent = NULL);
  ~pqIntRangeWidget();

  /// get the value
  int value() const;
  
  // get the min range value
  int minimum() const;
  // get the max range value
  int maximum() const;
  
signals:
  /// signal the value changed
  void valueChanged(int);

public slots:
  /// set the value
  void setValue(int);

  // set the min range value
  void setMinimum(int);
  // set the max range value
  void setMaximum(int);

private slots:
  void sliderChanged(int);
  void textChanged(const QString&);

private:
  int Value;
  int Minimum;
  int Maximum;
  QSlider* Slider;
  QLineEdit* LineEdit;
};

#endif

