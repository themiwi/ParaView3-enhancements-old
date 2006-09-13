/*=========================================================================

   Program:   ParaQ
   Module:    $RCSfile: pqStreamTracerPanel.h,v $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#ifndef _pqStreamTracerPanel_h
#define _pqStreamTracerPanel_h

#include "pqObjectPanel.h"
#include "pqObjectPanelInterface.h"

/// Custom panel for the StreamTracer filter that manages a combined Qt / 3D widget UI
class pqStreamTracerPanel :
  public pqObjectPanel
{
  typedef pqObjectPanel Superclass;

  Q_OBJECT

public:
  pqStreamTracerPanel(pqProxy& proxy, QWidget* p);
  ~pqStreamTracerPanel();
  
private slots:
  void onRenderModuleChanged(pqRenderModule*);
  void onSeedTypeChanged(int);

private:
  void onUsePointSource();
  void onUseLineSource();

  class pqImplementation;
  pqImplementation* const Implementation;
};

// make this panel available to the object inspector
class pqStreamTracerPanelInterface : public QObject, public pqObjectPanelInterface
{
  Q_OBJECT
  Q_INTERFACES(pqObjectPanelInterface)
public:
  virtual QString name() const;
  virtual pqObjectPanel* createPanel(pqProxy& proxy, QWidget* p);
  virtual bool canCreatePanel(pqProxy& proxy) const;
};

#endif
