/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqDisplayColorWidget.h,v $

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

#ifndef _pqDisplayColorWidget_h
#define _pqDisplayColorWidget_h

#include "pqVariableType.h"
#include "pqComponentsExport.h"

#include <QWidget>
#include <QPointer>

class QComboBox;
class QHBoxLayout;

class pqConsumerDisplay;
class pqPipelineDisplay;
class vtkEventQtSlotConnect;

/// Provides a standard user interface for selecting among a collection 
/// of dataset variables (both cell and node variables).
class PQCOMPONENTS_EXPORT pqDisplayColorWidget : public QWidget
{
  Q_OBJECT

public:
  pqDisplayColorWidget( QWidget *parent=0 );
  ~pqDisplayColorWidget();
  
  /// Removes all variables from the collection.
  void clear();

  /// Adds a variable to the collection.
  void addVariable(pqVariableType type, const QString& name);

  /// Makes the given variable the "current" selection.  Emits the 
  /// variableChanged() signal.
  void chooseVariable(pqVariableType type, const QString& name);

  pqPipelineDisplay* getDisplay() const;

  QString getCurrentText() const;

public slots:
  /// Called when the variable selection changes. 
  void onVariableChanged(pqVariableType type, const QString& name);

  /// When set, the source/renModule is not used to locate the
  /// display, instead this display is used.
  void setDisplay(pqConsumerDisplay* display);

signals:
  /// Signal emitted whenever the user chooses a variable, 
  /// or chooseVariable() is called.
  void variableChanged(pqVariableType type, const QString& name);

private slots:
  /// Called to emit the variableChanged() signal in response to user input 
  /// or the chooseVariable() method.
  void onVariableActivated(int row);

  /// Called when any important property on the display changes.
  /// This updates the selected value.
  void updateGUI();

  /// Called when the GUI must reload the arrays shown in the widget.
  void reloadGUI();

private:
  /// Converts a variable type and name into a packed string representation 
  /// that can be used with a combo box.
  static const QString variableData(pqVariableType, const QString& name);
 
  QIcon* CellDataIcon;
  QIcon* PointDataIcon;
  QIcon* SolidColorIcon;

  QHBoxLayout* Layout;
  QComboBox* Variables;
  bool BlockEmission;
  vtkEventQtSlotConnect* VTKConnect;
  QPointer<pqPipelineDisplay> Display;
  QList<QString> AvailableArrays;
};

#endif
