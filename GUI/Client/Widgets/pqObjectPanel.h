/*=========================================================================

   Program:   ParaQ
   Module:    $RCSfile: pqObjectPanel.h,v $

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

#ifndef _pqObjectPanel_h
#define _pqObjectPanel_h

#include <QWidget>
#include "pqSMProxy.h"

class pqPropertyManager;

/// Base class for Widget which provides an editor for editing properties of a proxy
class pqObjectPanel : public QWidget
{
  Q_OBJECT
public:
  /// constructor
  pqObjectPanel(QWidget* p);
  /// destructor
  ~pqObjectPanel();

  /// set the proxy to display properties for
  virtual void setProxy(pqSMProxy proxy);
  /// get the proxy for which properties are displayed
  virtual pqSMProxy proxy();
  
  /// size hint for this widget
  QSize sizeHint() const;

  /// property manager
  pqPropertyManager* getPropertyManager()
    { return this->PropertyManager; }

public slots:
  /// accept the changes made to the properties
  /// changes will be propogated down to the server manager
  void accept();
  /// reset the changes made
  /// editor will query properties from the server manager
  void reset();

protected:
  pqSMProxy Proxy;
  pqPropertyManager* PropertyManager;


};

#endif

