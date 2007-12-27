/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqSaveSnapshotDialog.h,v $

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

========================================================================*/
#ifndef __pqSaveSnapshotDialog_h 
#define __pqSaveSnapshotDialog_h

#include <QDialog>
#include "pqComponentsExport.h"

/// Dialog used to ask the user for the resolution of the snapshot to save.
class PQCOMPONENTS_EXPORT pqSaveSnapshotDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog Superclass;
public:
  pqSaveSnapshotDialog(QWidget* parent, Qt::WindowFlags f=0);
  ~pqSaveSnapshotDialog();

  /// Set the default size for the snapshot.
  void setViewSize(const QSize& size);
  QSize viewSize() const;

  /// Returns if the user requested to save all views.
  bool saveAllViews() const;
protected slots:
  /// Called when the user has edited width. If aspect ratio is locked,
  /// we will scale the height to maintain the aspect ration.
  void onWidthEdited();

  void onHeightEdited();

  void onLockAspectRatio(bool);

private:
  pqSaveSnapshotDialog(const pqSaveSnapshotDialog&); // Not implemented.
  void operator=(const pqSaveSnapshotDialog&); // Not implemented.

  class pqInternal;
  pqInternal* Internal;
};

#endif


