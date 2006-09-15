/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqScalarsToColors.h,v $

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
#ifndef __pqScalarsToColors_h
#define __pqScalarsToColors_h

#include "pqProxy.h"

class pqScalarsToColorsInternal;
class pqScalarBarDisplay;

/// pqScalarsToColors is a represents a vtkScalarsToColors proxy.
class PQCORE_EXPORT pqScalarsToColors : public pqProxy
{
  Q_OBJECT
public:
  pqScalarsToColors(const QString& group, const QString& name,
    vtkSMProxy* proxy, pqServer* server, QObject* parent=NULL);
  virtual ~pqScalarsToColors();

  // Returns the first scalar bar visible in the given render module,
  // if any.
  pqScalarBarDisplay* getScalarBar(pqRenderModule* ren) const;

public slots:
  // This method checks if this LUT is used by any display,
  // if not, it hides all the scalars bar showing this LUT.
  void hideUnusedScalarBars();

protected:
  friend class pqScalarBarDisplay;

  void addScalarBar(pqScalarBarDisplay*);
  void removeScalarBar(pqScalarBarDisplay*);

private:
  pqScalarsToColorsInternal* Internal;
};

#endif

