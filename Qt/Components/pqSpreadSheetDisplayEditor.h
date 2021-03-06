/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqSpreadSheetDisplayEditor.h,v $

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2. 

   See License_v1.2.txt for the full ParaView license.
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
#ifndef __pqSpreadSheetDisplayEditor_h 
#define __pqSpreadSheetDisplayEditor_h

#include "pqDisplayPanel.h"

/// Display properties page for a representation belonging to the spreadsheet
/// view.
class PQCOMPONENTS_EXPORT pqSpreadSheetDisplayEditor : public pqDisplayPanel
{
  Q_OBJECT
  typedef pqDisplayPanel Superclass;
public:
  pqSpreadSheetDisplayEditor(pqRepresentation* repr, QWidget* parent=0);
  virtual ~pqSpreadSheetDisplayEditor();

protected slots:
  /// Show/hide the process id controls based on whether mode 
  /// is "Field Data" or not.
  void onAttributeModeChanged(const QString &mode);

private:
  pqSpreadSheetDisplayEditor(const pqSpreadSheetDisplayEditor&); // Not implemented.
  void operator=(const pqSpreadSheetDisplayEditor&); // Not implemented.

  /// Internal method called during initialization.
  void setRepresentationInternal(pqRepresentation* repr);

  class pqInternal;
  pqInternal* Internal;
};

#endif


