/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqExodusPanel.h,v $

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
cxxPROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#ifndef _pqExodusPanel_h
#define _pqExodusPanel_h

#include "pqNamedObjectPanel.h"
#include "pqObjectPanelInterface.h"
class pqTreeWidgetItemObject;
class QTreeWidget;
class QListWidget;
class vtkPVArrayInformation;

class pqExodusPanel :
  public pqNamedObjectPanel
{
  Q_OBJECT
public:
  /// constructor
  pqExodusPanel(pqProxy* proxy, QWidget* p = NULL);
  /// destructor
  ~pqExodusPanel();

protected slots:
  void applyDisplacements(int);
  void displChanged(bool);

  void updateDataRanges();
  void propertyChanged();
  
  void blocksOn();
  void blocksOff();
  void blocksToggle(Qt::CheckState);
  
  void variablesOn();
  void variablesOff();
  void variablesToggle(Qt::CheckState);
  
  void setsOn();
  void setsOff();
  void setsToggle(Qt::CheckState);
  
  void toggle(QTreeWidget*, Qt::CheckState);
  void toggle(QListWidget*, Qt::CheckState);

protected:
  /// populate widgets with properties from the server manager
  virtual void linkServerManagerProperties();

  static QString formatDataFor(vtkPVArrayInformation* ai);

  pqTreeWidgetItemObject* DisplItem;

  bool DataUpdateInProgress;

  class pqUI;
  pqUI* UI;

};

// make this panel available to the object inspector
class pqExodusPanelInterface : public QObject, public pqObjectPanelInterface
{
  Q_OBJECT
  Q_INTERFACES(pqObjectPanelInterface)
public:
  virtual QString name() const;
  virtual pqObjectPanel* createPanel(pqProxy* proxy, QWidget* p);
  virtual bool canCreatePanel(pqProxy* proxy) const;
};

#endif

