/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqVCRController.h,v $

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

#ifndef _pqVCRController_h
#define _pqVCRController_h

#include "pqComponentsExport.h"

#include <QObject>
#include <QTimer>
class pqPipelineSource;

class PQCOMPONENTS_EXPORT pqVCRController : public QObject
{
  Q_OBJECT
public:
  pqVCRController(QObject* parent = 0);
  virtual ~pqVCRController();

signals:
  void timestepChanged();
  void playCompleted();

public slots:
  // Connect these signals to appropriate VCR buttons.
  void onFirstFrame();
  void onPreviousFrame();
  void onNextFrame();
  void onLastFrame();
  void onPlay();
  void onPause();

  void setSource(pqPipelineSource* source);

private slots:
  void onTimerNextFrame();
 
private:
  // internal method.
  bool updateSource(bool first, bool last, int offset);

  pqPipelineSource* Source;
  QTimer Timer;
};

#endif

