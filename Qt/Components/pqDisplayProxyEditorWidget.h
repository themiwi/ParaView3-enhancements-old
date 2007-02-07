/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqDisplayProxyEditorWidget.h,v $

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
#ifndef __pqDisplayProxyEditorWidget_h
#define __pqDisplayProxyEditorWidget_h

#include <QWidget>
#include "pqComponentsExport.h"

class pqDisplay;
class pqDisplayProxyEditorWidgetInternal;
class pqGenericViewModule;
class pqPipelineDisplay;
class pqPipelineSource;

// This is a widget that can create different kinds of display
// editors based on the type of the display. It encapsulates the code
// to decide what GUI for display editing must be shown to the user
// based on the type of the display.
class PQCOMPONENTS_EXPORT pqDisplayProxyEditorWidget : public QWidget
{
  Q_OBJECT
public:
  pqDisplayProxyEditorWidget(QWidget* parent=NULL);
  virtual ~pqDisplayProxyEditorWidget();

  /// Set the source and view. Source and view are used by this class
  /// only if display is NULL. It is used to decide if a new display 
  /// can be created for the source at all.
  void setSource(pqPipelineSource* source);
  void setView(pqGenericViewModule* view);

  /// Set the display to edit. If NULL, source and view must be set so
  /// that the widget can show a default GUI which allows the user to
  /// turn visibility on which entails creating a new display.
  void setDisplay(pqDisplay*);
  pqDisplay* getDisplay() const;

public slots:
  void reloadGUI();

signals:
  void requestReload();
  void requestSetDisplay(pqDisplay*);
  void requestSetDisplay(pqPipelineDisplay*);

protected slots:
  void onVisibilityChanged(int);

private:
  pqDisplayProxyEditorWidget(const pqDisplayProxyEditorWidget&); // Not implemented.
  void operator=(const pqDisplayProxyEditorWidget&); // Not implemented.

  pqDisplayProxyEditorWidgetInternal *Internal;
  void showDefaultWidget();
};

#endif

