/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqAnimationViewWidget.h,v $

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
#ifndef __pqAnimationViewWidget_h
#define __pqAnimationViewWidget_h

#include "pqComponentsExport.h"
#include <QWidget>

class pqAnimationCue;
class pqAnimationManager;
class pqAnimationScene;
class pqView;
class pqPipelineSource;
class pqProxy;
class pqServerManagerModelItem;
class QToolBar;
class vtkSMProxy;

/// This is the Animation panel widget. It controls the behaviour
/// of the Animation panel which includes adding of key frames,
/// changing of keyframes etc etc.
class PQCOMPONENTS_EXPORT pqAnimationViewWidget : public QWidget
{
  Q_OBJECT
public:
  pqAnimationViewWidget(QWidget* parent);
  virtual ~pqAnimationViewWidget();

  typedef QWidget Superclass;
  class pqInternals;

  /// Set the animation manager to use.
  void setManager(pqAnimationManager* mgr);

protected slots:
  /// Called when the active scene changes.
  void onActiveSceneChanged(pqAnimationScene* scene);

  /// The cues in the scene have changed, so we make sure
  /// that we are not displaying a removed or added cue, if so
  /// we update the GUI.
  void onSceneCuesChanged();

  /// called when keyframes change
  void keyFramesChanged(QObject*);
  
private:
  pqAnimationViewWidget(const pqAnimationViewWidget&); // Not implemented.
  void operator=(const pqAnimationViewWidget&); // Not implemented.

  //void setActiveCue(pqAnimationCue*);
  pqInternals *Internal;
};

#endif

