/*=========================================================================

   Program:   ParaQ
   Module:    $RCSfile: pqSMAdaptor.h,v $

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

#ifndef _pqSMAdaptor_h
#define _pqSMAdaptor_h

class pqSMAdaptorInternal;
class vtkSMProperty;
class vtkSMProxy;
class vtkObject;
class QWidget;

#include "pqWidgetsExport.h"
#include <QObject>
#include <QVariant>
#include <QList>
#include "pqSMProxy.h"

/// Translates server manager events into Qt-compatible slots and signals
class PQWIDGETS_EXPORT pqSMAdaptor : public QObject
{
protected: 
  // class not instantiated
  pqSMAdaptor();
  ~pqSMAdaptor();

public:

  /// enumeration for types of properties this class handles
  enum PropertyType
    {
    UNKNOWN,
    PROXY,
    PROXYLIST,
    SELECTION,
    ENUMERATION,
    SINGLE_ELEMENT,
    MULTIPLE_ELEMENTS,
    FILE_LIST
    };

  /// Get the type of the property
  static PropertyType getPropertyType(vtkSMProperty* Property);

  /// get the proxy for a property
  /// for example, glyph filter accepts a source (proxy) to glyph with
  static pqSMProxy getProxyProperty(vtkSMProperty* Property);
  /// get the proxy for a property
  /// for example, glyph filter accepts a source (proxy) to glyph with
  static void setProxyProperty(vtkSMProperty* Property, 
                               pqSMProxy Value);
  static void setUncheckedProxyProperty(vtkSMProperty* Property, 
                                        pqSMProxy Value);
  
  /// get the list of proxies for a property
  /// for example, append filter accepts a list of proxies
  static QList<pqSMProxy> getProxyListProperty(vtkSMProperty* Property);
  /// get the list of proxies for a property
  /// for example, append filter accepts a list of proxies
  static void setProxyListProperty(vtkSMProperty* Property, 
                                   QList<pqSMProxy> Value);

  /// get the list of possible proxies for a property
  static QList<pqSMProxy> getProxyPropertyDomain(vtkSMProperty* Property);


  /// get the pairs of selections for a selection property
  static QList<QList<QVariant> > getSelectionProperty(vtkSMProperty* Property);
  /// get the pairs of selections for a selection property
  static QList<QVariant> getSelectionProperty(vtkSMProperty* Property, 
                                              unsigned int Index);
  /// set the pairs of selections for a selection property
  static void setSelectionProperty(vtkSMProperty* Property, 
                                   QList<QList<QVariant> > Value);
  static void setUncheckedSelectionProperty(vtkSMProperty* Property, 
                                            QList<QList<QVariant> > Value);
  /// set the pairs of selections for a selection property
  static void setSelectionProperty(vtkSMProperty* Property, 
                                   QList<QVariant> Value);
  static void setUncheckedSelectionProperty(vtkSMProperty* Property, 
                                            QList<QVariant> Value);
  /// get the possible names for the selection property
  static QList<QVariant> getSelectionPropertyDomain(vtkSMProperty* Property);
  
  /// get the enumeration for a property
  static QVariant getEnumerationProperty(vtkSMProperty* Property);
  /// set the enumeration for a property
  static void setEnumerationProperty(vtkSMProperty* Property, 
                                     QVariant Value);
  static void setUncheckedEnumerationProperty(vtkSMProperty* Property, 
                                              QVariant Value);
  /// get the possible enumerations (string) for a property
  static QList<QVariant> getEnumerationPropertyDomain(vtkSMProperty* Property);

  /// get the single element of a property (integer, string, real, etc..)
  static QVariant getElementProperty(vtkSMProperty* Property);
  /// set the single element of a property (integer, string, real, etc..)
  static void setElementProperty(vtkSMProperty* Property, 
                                 QVariant Value);
  static void setUncheckedElementProperty(vtkSMProperty* Property, 
                                          QVariant Value);
  /// get the range of possible values to set the single element of a property
  static QList<QVariant> getElementPropertyDomain(vtkSMProperty* Property);
  
  /// get the multiple elements of a property (integer, string, real, etc..)
  static QList<QVariant> getMultipleElementProperty(vtkSMProperty* Property);
  /// set the multiple elements of a property (integer, string, real, etc..)
  static void setMultipleElementProperty(vtkSMProperty* Property, 
                                         QList<QVariant> Value);
  static void setUncheckedMultipleElementProperty(vtkSMProperty* Property, 
                                                  QList<QVariant> Value);
  /// get the ranges of possible values to 
  /// set the multiple elements of a property
  static QList<QList<QVariant> > getMultipleElementPropertyDomain(
                                           vtkSMProperty* Property);
  /// get one of the multiple elements of a 
  /// property (integer, string, real, etc..)
  static QVariant getMultipleElementProperty(vtkSMProperty* Property, 
                                             unsigned int Index);
  /// set one of the multiple elements of a 
  /// property (integer, string, real, etc..)
  static void setMultipleElementProperty(vtkSMProperty* Property, 
                                         unsigned int Index, 
                                         QVariant Value);
  static void setUncheckedMultipleElementProperty(vtkSMProperty* Property, 
                                                  unsigned int Index, 
                                                  QVariant Value);
  /// get one of the ranges of possible values 
  /// to set the multiple elements of a property
  static QList<QVariant> getMultipleElementPropertyDomain(
                       vtkSMProperty* Property, unsigned int Index);

  /// get the single element of a property (integer, string, real, etc..)
  static QString getFileListProperty(vtkSMProperty* Property);
  /// set the single element of a property (integer, string, real, etc..)
  static void setFileListProperty(vtkSMProperty* Property, 
                                  QString Value);
  static void setUncheckedFileListProperty(vtkSMProperty* Property, 
                                           QString Value);

};

#endif // !_pqSMAdaptor_h

