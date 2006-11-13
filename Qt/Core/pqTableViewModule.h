/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqTableViewModule.h,v $

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
#ifndef __pqTableViewModule_h
#define __pqTableViewModule_h

#include "pqGenericViewModule.h"

class PQCORE_EXPORT pqTableViewModule :
  public pqGenericViewModule
{
  typedef pqGenericViewModule Superclass;

  Q_OBJECT
public:
  pqTableViewModule(const QString& group, const QString& name, 
    vtkSMAbstractViewModuleProxy* renModule, 
    pqServer* server, QObject* parent=NULL);
  ~pqTableViewModule();

  QWidget* getWidget();

  /// Call this method to assign a Window in which this view module will
  /// be displayed.
  virtual void setWindowParent(QWidget* parent);
  virtual QWidget* getWindowParent() const;

  /// Save a screenshot for the render module. If width or height ==0,
  /// the current window size is used.
  virtual bool saveImage(int /*width*/, int /*height*/, 
    const QString& /*filename*/) 
    {
    // Not supported yet.
    return false;
    };

  /// This method returns is any pqPipelineSource can be dislayed in this
  /// view. Overridden to make sure that the source can be displayed
  /// in this type of plot.
  virtual bool canDisplaySource(pqPipelineSource* source) const;

  /// Forces an immediate render. Overridden since for plots
  /// rendering actually happens on the GUI side, not merely
  /// in the ServerManager.
  virtual void forceRender();

private slots:
  void visibilityChanged(pqDisplay* disp);

protected:

private:
  pqTableViewModule(const pqTableViewModule&); // Not implemented.
  void operator=(const pqTableViewModule&); // Not implemented.

  class pqImplementation;
  pqImplementation* const Implementation;
};


#endif

