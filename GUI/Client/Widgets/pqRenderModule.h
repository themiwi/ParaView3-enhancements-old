/*=========================================================================

   Program:   ParaQ
   Module:    $RCSfile: pqRenderModule.h,v $

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
#ifndef __pqRenderModule_h
#define __pqRenderModule_h


#include "pqProxy.h"

class pqRenderModuleInternal;
class pqServer;
class QVTKWidget;
class QWidget;
class vtkSMRenderModuleProxy;


// This is a PQ abstraction of a render module.
class PQWIDGETS_EXPORT pqRenderModule : public pqProxy
{
  Q_OBJECT
public:
  pqRenderModule(const QString& name, vtkSMRenderModuleProxy* renModule, 
    pqServer* server, QObject* parent=NULL);
  virtual ~pqRenderModule();

  /// Returns the internal render Module proxy associated with this object.
  vtkSMRenderModuleProxy* getProxy() const;

  /// Returns the QVTKWidget for this render Window.
  QVTKWidget* getWidget() const;

  /// Call this method to assign a Window in which this render module will render.
  /// This will set the QVTKWidget's parent.
  void setWindowParent(QWidget* parent);

  /// Request a StillRender.
  void render();

  /// Resets the camera to include all visible data.
  void resetCamera();

  /// Returns the name for this render module.
  const QString &getProxyName() const; 

  /// Save a screenshot for the render module. If width or height ==0,
  /// the current window size is used.
  bool saveImage(int width, int height, const QString& filename);

private slots:
  /// if renModule is not created when this object is instantianted, we
  /// must listen to UpdateVTKObjects event to bind the QVTKWidget and
  /// then render window.
  void onUpdateVTKObjects();

protected:
  // Event filter callback.
  bool eventFilter(QObject* caller, QEvent* e);

private:
  /// setups up RM and QVTKWidget binding.
  void renderModuleInit();
  
  pqRenderModuleInternal* Internal;
};

#endif

