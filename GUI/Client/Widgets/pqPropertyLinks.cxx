/*=========================================================================

   Program:   ParaQ
   Module:    $RCSfile: pqPropertyLinks.cxx,v $

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

// self include
#include "pqPropertyLinks.h"

// standard includes
#include <vtkstd/set>

// Qt includes
#include <QString>
#include <QPointer>
#include <QVariant>
#include <QSignalMapper>

// VTK includes
#include "vtkEventQtSlotConnect.h"

// ParaQ includes
#include "vtkSMProperty.h"
#include "pqSMAdaptor.h"


static bool SettingValue = false;

class pqPropertyLinksConnection::pqInternal
{
public:
  pqInternal()
  {
    this->UseUncheckedProperties = false;
  }

  pqSMAdaptor::PropertyType Type;

  vtkSMProxy* Proxy;
  vtkSMProperty* Property;
  int Index;

  QPointer<QObject> QtObject;
  QByteArray QtProperty;
  bool UseUncheckedProperties;

  // This flag indicates if the QObject and the vtkSMProperty are out of synch. 
  bool OutOfSync;
};

class pqPropertyLinks::pqInternal
{
public:
  pqInternal()
  {
    this->VTKConnections = vtkEventQtSlotConnect::New();
    this->UseUncheckedProperties = false;
  }
  ~pqInternal()
  {
    this->VTKConnections->Delete();
  }

  // handle changes from the SM side
  vtkEventQtSlotConnect* VTKConnections;
  
  typedef vtkstd::multiset<pqPropertyLinksConnection> LinkMap;
  LinkMap Links;
  bool UseUncheckedProperties;
};


pqPropertyLinksConnection::pqPropertyLinksConnection(vtkSMProxy* smproxy, 
                                      vtkSMProperty* smproperty, 
                                      int idx,
                                      QObject* qobject, 
                                      const char* qproperty)
{
  this->Internal = new pqPropertyLinksConnection::pqInternal;
  this->Internal->Proxy = smproxy;
  this->Internal->Property = smproperty;
  this->Internal->Index = idx;
  this->Internal->QtObject = qobject;
  this->Internal->QtProperty = qproperty;
  this->Internal->Type = pqSMAdaptor::getPropertyType(this->Internal->Property);
  this->Internal->OutOfSync = false;
}

pqPropertyLinksConnection::pqPropertyLinksConnection(
      const pqPropertyLinksConnection& copy)
  : QObject()
{
  this->Internal = new pqPropertyLinksConnection::pqInternal;

  this->Internal->Type = copy.Internal->Type;

  this->Internal->Proxy = copy.Internal->Proxy;
  this->Internal->Property = copy.Internal->Property;
  this->Internal->Index = copy.Internal->Index;
  
  this->Internal->QtObject = copy.Internal->QtObject;
  this->Internal->QtProperty = copy.Internal->QtProperty;
  
  this->Internal->UseUncheckedProperties = 
      copy.Internal->UseUncheckedProperties;

  this->Internal->OutOfSync = copy.Internal->OutOfSync;
}

pqPropertyLinksConnection::~pqPropertyLinksConnection()
{
  delete this->Internal;
}

bool pqPropertyLinksConnection::getOutOfSync() const
{
  return this->Internal->OutOfSync;
}


pqPropertyLinksConnection& pqPropertyLinksConnection::operator=(
     const pqPropertyLinksConnection& copy)
{
  this->Internal->Type = copy.Internal->Type;

  this->Internal->Proxy = copy.Internal->Proxy;
  this->Internal->Property = copy.Internal->Property;
  this->Internal->Index = copy.Internal->Index;
  
  this->Internal->QtObject = copy.Internal->QtObject;
  this->Internal->QtProperty = copy.Internal->QtProperty;
  
  this->Internal->UseUncheckedProperties = 
    copy.Internal->UseUncheckedProperties;
 
  this->Internal->OutOfSync = copy.Internal->OutOfSync;
  return *this;
}
bool pqPropertyLinksConnection::operator<(
    pqPropertyLinksConnection const& other) const
{
  if(this->Internal->Proxy < other.Internal->Proxy)
    {
    return true;
    }
  else if(this->Internal->Proxy > other.Internal->Proxy)
    {
    return false;
    }

  if(this->Internal->Property < other.Internal->Property)
    {
    return true;
    }
  else if(this->Internal->Property > other.Internal->Property)
    {
    return false;
    }
  
  if(this->Internal->Index < other.Internal->Index)
    {
    return true;
    }
  if(this->Internal->Index > other.Internal->Index)
    {
    return false;
    }
  
  if(this->Internal->QtObject < other.Internal->QtObject)
    {
    return true;
    }
  if(this->Internal->QtObject > other.Internal->QtObject)
    {
    return false;
    }
  
  return this->Internal->QtProperty < other.Internal->QtProperty;
}

void pqPropertyLinksConnection::smLinkedPropertyChanged() const
{
  if(SettingValue)
    {
    return;
    }
  this->Internal->OutOfSync = true;
  SettingValue = true;

  if(this->Internal->QtObject)
    {
    // get the property of the object
    QVariant old;
    old = this->Internal->QtObject->property(this->Internal->QtProperty);
    QVariant prop;
    switch(this->Internal->Type)
      {
      case pqSMAdaptor::PROXY:
        {
        pqSMProxy p;
        p = pqSMAdaptor::getProxyProperty(this->Internal->Proxy, 
                  this->Internal->Property);
        prop.setValue(p);
        if(prop != old)
          {
          this->Internal->QtObject->setProperty(this->Internal->QtProperty, 
                                                prop);
          }
        }
      break;
      case pqSMAdaptor::ENUMERATION:
        {
        prop = pqSMAdaptor::getEnumerationProperty(this->Internal->Proxy, 
                 this->Internal->Property);
        if(prop != old)
          {
          this->Internal->QtObject->setProperty(this->Internal->QtProperty, 
                                                prop);
          }
        }
      break;
      case pqSMAdaptor::SINGLE_ELEMENT:
        {
        prop = pqSMAdaptor::getElementProperty(this->Internal->Proxy, 
                                          this->Internal->Property);
        if(prop != old)
          {
          this->Internal->QtObject->setProperty(this->Internal->QtProperty, 
                                                prop);
          }
        }
      break;
      case pqSMAdaptor::FILE_LIST:
        {
        prop = pqSMAdaptor::getFileListProperty(this->Internal->Proxy, 
                  this->Internal->Property);
        if(prop != old)
          {
          this->Internal->QtObject->setProperty(this->Internal->QtProperty,
                                               prop);
          }
        }
      break;
      case pqSMAdaptor::SELECTION:
        {
        QList<QVariant> sel;
        sel = pqSMAdaptor::getSelectionProperty(this->Internal->Proxy, 
                                    this->Internal->Property, 
                                    this->Internal->Index);

        if(sel.size() == 2 && sel[1] != old)
          {
          this->Internal->QtObject->setProperty(this->Internal->QtProperty, 
                                                sel[1]);
          }
        }
      break;
      case pqSMAdaptor::MULTIPLE_ELEMENTS:
        {
        if(this->Internal->Index == -1)
          {
          prop = pqSMAdaptor::getMultipleElementProperty(this->Internal->Proxy, 
                                 this->Internal->Property);
          if(prop != old)
            {
            this->Internal->QtObject->setProperty(this->Internal->QtProperty, 
                                prop);
            }
          }
        else
          {
          prop = pqSMAdaptor::getMultipleElementProperty(this->Internal->Proxy, 
                                   this->Internal->Property, 
                                   this->Internal->Index);
          if(prop != old)
            {
            this->Internal->QtObject->setProperty(this->Internal->QtProperty, 
                                    prop);
            }
          }
        }
      break;
      case pqSMAdaptor::UNKNOWN:
      case pqSMAdaptor::PROXYLIST:
      break;
      }
    }
  SettingValue = false;
}

void pqPropertyLinksConnection::qtLinkedPropertyChanged() const
{
  if(SettingValue)
    {
    return;
    }
  this->Internal->OutOfSync = true;
  SettingValue = true;

  if(this->Internal->QtObject)
    {
    // get the property of the object
    QVariant prop;
    prop = this->Internal->QtObject->property(this->Internal->QtProperty);
    QVariant old;
    switch(this->Internal->Type)
      {
      case pqSMAdaptor::PROXY:
        {
        if(this->Internal->UseUncheckedProperties)
          {
          pqSMAdaptor::setUncheckedProxyProperty(this->Internal->Proxy, 
                this->Internal->Property, prop.value<pqSMProxy>());
          }
        else
          {
          pqSMProxy p;
          p = pqSMAdaptor::getProxyProperty(this->Internal->Proxy, 
                                            this->Internal->Property);
          old.setValue(p);
          if(prop != old)
            {
            pqSMAdaptor::setProxyProperty(this->Internal->Proxy, 
                  this->Internal->Property, prop.value<pqSMProxy>());
            }
          }
        }
      break;
      case pqSMAdaptor::ENUMERATION:
        {
        if(this->Internal->UseUncheckedProperties)
          {
          pqSMAdaptor::setUncheckedEnumerationProperty(this->Internal->Proxy, 
                                  this->Internal->Property, prop);
          }
        else
          {
          old = pqSMAdaptor::getEnumerationProperty(this->Internal->Proxy, 
                                this->Internal->Property);
          if(prop != old)
            {
            pqSMAdaptor::setEnumerationProperty(this->Internal->Proxy, 
                                    this->Internal->Property, prop);
            }
          }
        }
      break;
      case pqSMAdaptor::SINGLE_ELEMENT:
        {
        if(this->Internal->UseUncheckedProperties)
          {
          pqSMAdaptor::setUncheckedElementProperty(this->Internal->Proxy, 
                               this->Internal->Property, prop);
          }
        else
          {
          old = pqSMAdaptor::getElementProperty(this->Internal->Proxy, 
                                        this->Internal->Property);
          if(prop != old)
            {
            pqSMAdaptor::setElementProperty(this->Internal->Proxy, 
                                 this->Internal->Property, prop);
            }
          }
        }
      break;
      case pqSMAdaptor::FILE_LIST:
        {
        if(this->Internal->UseUncheckedProperties)
          {
          pqSMAdaptor::setUncheckedFileListProperty(this->Internal->Proxy, 
                                  this->Internal->Property, prop.toString());
          }
        else
          {
          old = pqSMAdaptor::getFileListProperty(this->Internal->Proxy, 
                                   this->Internal->Property);
          if(prop != old)
            {
            pqSMAdaptor::setFileListProperty(this->Internal->Proxy, 
                                    this->Internal->Property, prop.toString());
            }
          }
        }
      break;
      case pqSMAdaptor::SELECTION:
        {
        if(this->Internal->UseUncheckedProperties)
          {
          QList<QVariant> sel;
          sel = pqSMAdaptor::getSelectionProperty(this->Internal->Proxy, 
                                               this->Internal->Property, 
                                               this->Internal->Index);
          if(sel.size() == 2)
            {
            sel[1] = prop;
            pqSMAdaptor::setUncheckedSelectionProperty(this->Internal->Proxy, 
                         this->Internal->Property, sel);
            }
          }
        else
          {
          QList<QVariant> sel;
          sel = pqSMAdaptor::getSelectionProperty(this->Internal->Proxy, 
                                               this->Internal->Property, 
                                               this->Internal->Index);
          if(sel.size() == 2 && prop != sel[1])
            {
            sel[1] = prop;
            pqSMAdaptor::setSelectionProperty(this->Internal->Proxy, 
                         this->Internal->Property, sel);
            }
          }
        }
      break;
      case pqSMAdaptor::MULTIPLE_ELEMENTS:
        {
        if(this->Internal->Index == -1)
          {
          if(this->Internal->UseUncheckedProperties)
            {
            pqSMAdaptor::setUncheckedMultipleElementProperty(
                            this->Internal->Proxy, 
                            this->Internal->Property, prop.toList());
            }
          else
            {
            old = pqSMAdaptor::getMultipleElementProperty(this->Internal->Proxy, 
                                   this->Internal->Property);
            if(prop != old)
              {
              pqSMAdaptor::setMultipleElementProperty(this->Internal->Proxy, 
                              this->Internal->Property, prop.toList());
              }
            }
          }
        else
          {
          if(this->Internal->UseUncheckedProperties)
            {
            pqSMAdaptor::setUncheckedMultipleElementProperty(
                      this->Internal->Proxy, 
                      this->Internal->Property, this->Internal->Index, prop);
            }
          else
            {
            old = pqSMAdaptor::getMultipleElementProperty(this->Internal->Proxy, 
                            this->Internal->Property, this->Internal->Index);
            if(prop != old)
              {
              pqSMAdaptor::setMultipleElementProperty(this->Internal->Proxy, 
                        this->Internal->Property, this->Internal->Index, prop);
              }
            }
          }
        }
      break;
      case pqSMAdaptor::UNKNOWN:
      case pqSMAdaptor::PROXYLIST:
      break;
      }
    }
  SettingValue = false;
}

bool pqPropertyLinksConnection::useUncheckedProperties() const
{
  return this->Internal->UseUncheckedProperties;
}

void pqPropertyLinksConnection::setUseUncheckedProperties(bool flag) const
{
  this->Internal->UseUncheckedProperties = flag;
}

class pqPropertyLinksInternal
{
public:
  pqPropertyLinksInternal()
    {
    this->VTKConnections = vtkEventQtSlotConnect::New();
    }
  ~pqPropertyLinksInternal()
    {
    this->VTKConnections->Delete();
    }

  // handle changes from the SM side
  vtkEventQtSlotConnect* VTKConnections;
  typedef vtkstd::multiset<pqPropertyLinksConnection> LinkMap;
  LinkMap Links;
};

pqPropertyLinks::pqPropertyLinks(QObject* p)
  : QObject(p)
{
  this->Internal = new pqPropertyLinks::pqInternal;
}

pqPropertyLinks::~pqPropertyLinks()
{
  delete this->Internal;
}

void pqPropertyLinks::addPropertyLink(QObject* qObject, const char* qProperty, 
                                      const char* signal,
                                      vtkSMProxy* Proxy, 
                                      vtkSMProperty* Property, int Index)
{
  if(!Proxy || !Property || !qObject || !qProperty || !signal)
    {
    return;
    }
  
  pqPropertyLinksInternal::LinkMap::iterator iter;
  pqPropertyLinksConnection conn(Proxy, Property, Index, qObject, qProperty);
  iter = this->Internal->Links.insert(conn);
  this->Internal->VTKConnections->Connect(Property, vtkCommand::ModifiedEvent,
                                          &*iter, 
                                          SLOT(smLinkedPropertyChanged()));
  
  QObject::connect(qObject, signal, &*iter, SLOT(qtLinkedPropertyChanged()));
  
  iter->setUseUncheckedProperties(this->Internal->UseUncheckedProperties);
  // set the object property to the current server manager property value
  iter->smLinkedPropertyChanged();
}

//-----------------------------------------------------------------------------
void pqPropertyLinks::removePropertyLink(QObject* qObject, 
                        const char* qProperty, const char* signal,
                        vtkSMProxy* Proxy, vtkSMProperty* Property, int Index)
{
  pqPropertyLinksConnection conn(Proxy, Property, Index, qObject, qProperty);
  pqPropertyLinksInternal::LinkMap::iterator iter;
  iter = this->Internal->Links.find(conn);
  if(iter != this->Internal->Links.end())
    {
    this->Internal->VTKConnections->Disconnect(iter->Internal->Property, 
                                  vtkCommand::ModifiedEvent, qObject);
    QObject::disconnect(iter->Internal->QtObject, signal, &*iter, 
                        SLOT(qtLinkedPropertyChanged()));
    }
}

//-----------------------------------------------------------------------------
void pqPropertyLinks::removeAllPropertyLinks()
{
  pqPropertyLinksInternal::LinkMap::iterator iter;
  for (iter = this->Internal->Links.begin(); iter != this->Internal->Links.end();
    ++iter)
    {
    this->Internal->VTKConnections->Disconnect(iter->Internal->Property);
    QObject::disconnect(iter->Internal->QtObject, 0, &*iter, 0);
    }
  this->Internal->Links.clear();
}

//-----------------------------------------------------------------------------
void pqPropertyLinks::reset()
{
  pqPropertyLinksInternal::LinkMap::iterator iter;
  for(iter = this->Internal->Links.begin();
      iter != this->Internal->Links.end();
      ++iter)
    {
    if (iter->getOutOfSync())
      {
      iter->smLinkedPropertyChanged();
      }
    }
}

void pqPropertyLinks::accept()
{
  bool old = this->useUncheckedProperties();

  pqPropertyLinksInternal::LinkMap::iterator iter;
  for(iter = this->Internal->Links.begin();
      iter != this->Internal->Links.end();
      ++iter)
    {
    if (!iter->getOutOfSync())
      {
      continue;
      }
    iter->setUseUncheckedProperties(false);
    iter->qtLinkedPropertyChanged();
    iter->setUseUncheckedProperties(old);
    }
}

bool pqPropertyLinks::useUncheckedProperties()
{
  return this->Internal->UseUncheckedProperties;
}

void pqPropertyLinks::setUseUncheckedProperties(bool flag)
{
  this->Internal->UseUncheckedProperties = flag;
  
  pqPropertyLinksInternal::LinkMap::iterator iter;
  for(iter = this->Internal->Links.begin();
      iter != this->Internal->Links.end();
      ++iter)
    {
    iter->setUseUncheckedProperties(flag);
    }
}

