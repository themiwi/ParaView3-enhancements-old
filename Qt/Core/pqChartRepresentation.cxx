/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqChartRepresentation.cxx,v $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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
#include "pqChartRepresentation.h"

#include "pqSMAdaptor.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"

#include <QString>
#include <QRegExp>

//-----------------------------------------------------------------------------
pqChartRepresentation::pqChartRepresentation(
  const QString& group, const QString& name,
  vtkSMProxy* reprProxy, pqServer* server,
  QObject* parentObject)
: Superclass(group, name, reprProxy, server, parentObject)
{
}

//-----------------------------------------------------------------------------
pqChartRepresentation::~pqChartRepresentation()
{
}

//-----------------------------------------------------------------------------
void pqChartRepresentation::setDefaultPropertyValues()
{
  this->Superclass::setDefaultPropertyValues();
  if (!this->isVisible())
    {
    return;
    }

  vtkSMProxy* proxy = this->getProxy();

  // * Determine the x-axis array: are we using index or do we have some array
  //   that suits out purpose better?
  QList<QVariant> series_arrays = pqSMAdaptor::getMultipleElementProperty(
    proxy->GetProperty("SeriesNamesInfo"));
  QString x_array;
  if (series_arrays.contains("Time"))
    {
    x_array = "Time";
    }
  else if (series_arrays.contains("arc_length"))
    {
    x_array = "arc_length";
    }

  if (!x_array.isEmpty())
    {
    vtkSMPropertyHelper(proxy, "XArrayName").Set(x_array.toAscii().data());
    vtkSMPropertyHelper(proxy, "UseIndexForXAxis").Set(0);
    }

  // * Turn off Y-series that don't make much sense to show by default.
  if (proxy->GetProperty("SeriesVisibility"))
    {
    vtkSMPropertyHelper helper(proxy, "SeriesVisibility");
    foreach (QVariant varray, series_arrays)
      {
      QString array = varray.toString();
      if (array.contains(QRegExp("\\(d+\\)$")))
        {
        // by default only "vector magnitudes" are plotted, not the components.
        helper.SetStatus(array.toAscii().data(), 0);
        }
      else if (array == "vtkValidPointMask")
        {
        helper.SetStatus(array.toAscii().data(), 0);
        }
      else if (array == "arc_length")
        {
        helper.SetStatus(array.toAscii().data(), 0);
        }
      else if (array.contains(QRegExp("^Points")))
        {
        helper.SetStatus(array.toAscii().data(), 0);
        }
      }
    }
  
  proxy->UpdateVTKObjects();
}
