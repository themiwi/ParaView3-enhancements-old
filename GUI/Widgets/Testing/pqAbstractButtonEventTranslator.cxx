/*=========================================================================

   Program:   ParaQ
   Module:    $RCSfile: pqAbstractButtonEventTranslator.cxx,v $

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

#include "pqAbstractButtonEventTranslator.h"

#include <QAbstractButton>
#include <QAction>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QToolButton>

#include <iostream>

pqAbstractButtonEventTranslator::pqAbstractButtonEventTranslator()
{
}

bool pqAbstractButtonEventTranslator::translateEvent(QObject* Object, QEvent* Event, bool& /*Error*/)
{
  QAbstractButton* const object = qobject_cast<QAbstractButton*>(Object);
  if(!object)
    return false;
    
  switch(Event->type())
    {
    case QEvent::KeyPress:
      {
      QKeyEvent* const event = static_cast<QKeyEvent*>(Event);
      if(event->key() == Qt::Key_Space)
        {
        onActivate(object);
        }
      }
      break;
    case QEvent::MouseButtonRelease:
      {
      QMouseEvent* const event = static_cast<QMouseEvent*>(Event);
      if(event->button() == Qt::LeftButton && object->rect().contains(event->pos()))
        {
        onActivate(object);
        }
      }
      break;
    default:
      break;
    }
      
  return true;
}

void pqAbstractButtonEventTranslator::onActivate(QAbstractButton* object)
{
  if(object->isCheckable())
    {
    const bool new_value = !object->isChecked();
    emit recordEvent(object, "set_boolean", new_value ? "true" : "false");
    }
  else
    {
    if(object->objectName() == QString::null && qobject_cast<QToolButton*>(object))
      {
      emit recordEvent(qobject_cast<QToolButton*>(object)->defaultAction(), "activate", "");
      }
    else
      {
      emit recordEvent(object, "activate", "");
      }
    }
}
