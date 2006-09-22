/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqSourceDisplayEditor.h,v $

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
#ifndef _pqSourceDisplayProxy_h
#define _pqSourceDisplayProxy_h

#include <QWidget>
#include <QPointer>
#include "pqComponentsExport.h"

class pqProxy;
class pqPipelineSource;
class pqPipelineDisplay;
class QComboBox;
class QStackedWidget;

/// shows the displays for a source
class PQCOMPONENTS_EXPORT pqSourceDisplayEditor : public QWidget
{
  Q_OBJECT
public:
  /// constructor
  pqSourceDisplayEditor(QWidget* p=0);
  /// destructor
  ~pqSourceDisplayEditor();
  
  /// get the proxy for which properties are displayed
  pqProxy* getProxy();

public slots:
  /// Set the display whose properties we want to edit. 
  void setProxy(pqProxy* source);

protected slots:
  void displayAdded(pqPipelineSource* source, pqPipelineDisplay* display);
  void displayRemoved(pqPipelineSource* source, pqPipelineDisplay* display);
  void proxyDestroyed();
  void changePage();

protected:
  void removePage(pqPipelineDisplay*);
  void addPage(pqPipelineDisplay*);

private:
  QStackedWidget* StackWidget;
  QComboBox* DisplayCombo;
  QPointer<pqProxy> Proxy;

};

#endif

