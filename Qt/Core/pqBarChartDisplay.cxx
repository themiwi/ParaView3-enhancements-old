/*=========================================================================

   Program: ParaView
   Module:    $RCSfile: pqBarChartDisplay.cxx,v $

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
#include "pqBarChartDisplay.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkPointData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSMGenericViewDisplayProxy.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMStringVectorProperty.h"

#include <QtDebug>

#include "pqApplicationCore.h"
#include "pqLookupTableManager.h"
#include "pqScalarsToColors.h"
#include "pqSMAdaptor.h"

//-----------------------------------------------------------------------------
pqBarChartDisplay::pqBarChartDisplay(const QString& group, const QString& name,
  vtkSMProxy* display, pqServer* server, QObject* _parent)
: pqConsumerDisplay(group, name, display, server, _parent)
{

}

//-----------------------------------------------------------------------------
pqBarChartDisplay::~pqBarChartDisplay()
{
}

//-----------------------------------------------------------------------------
pqScalarsToColors* pqBarChartDisplay::setLookupTable(const char* arrayname)
{
  // Now set up default lookup table.
  pqApplicationCore* core = pqApplicationCore::instance();
  pqLookupTableManager* lut_mgr = core->getLookupTableManager();
  vtkSMProxy* lut = 0;
  pqScalarsToColors* pqlut = lut_mgr->getLookupTable(
    this->getServer(), arrayname, 1, 0);
  lut = (pqlut)? pqlut->getProxy() : 0;

  vtkSMProxy* proxy = this->getProxy();
  pqSMAdaptor::setProxyProperty(
    proxy->GetProperty("LookupTable"), lut);
  proxy->UpdateVTKObjects();

  return pqlut;
}

//-----------------------------------------------------------------------------
void pqBarChartDisplay::setDefaults()
{
  this->Superclass::setDefaults();
  if (!this->isVisible())
    {
    // For any non-visible display, we don't set its defaults.
    return;
    }

  // Set default arrays and lookup table.
  vtkSMGenericViewDisplayProxy* proxy = 
    vtkSMGenericViewDisplayProxy::SafeDownCast(this->getProxy());
  proxy->GetProperty("Input")->UpdateDependentDomains();

  // setDefaults() can always call Update on the display. 
  // This is safe since setDefaults() will typically be called only after having
  // added the display to the render module, which ensures that the
  // update time has been set correctly on the display.
  proxy->Update();

  // This will setup default array names. Just reset-to-default all properties,
  // the vtkSMArrayListDomain will do the rest.
  vtkSMPropertyIterator* iter = proxy->NewPropertyIterator();
  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    iter->GetProperty()->ResetToDefault();
    }
  iter->Delete();

  // By default, we use the 1st point array as the X axis. If no point data
  // is present we use the X coordinate of the points themselves.
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(
    proxy->GetProperty("XArrayName"));
  bool use_points = (svp->GetElement(0) == 0);
  pqSMAdaptor::setElementProperty(
    proxy->GetProperty("XAxisUsePoints"), use_points);
  proxy->UpdateVTKObjects();

  // Now initialize the lookup table.
  this->updateLookupTable();
}

//-----------------------------------------------------------------------------
void pqBarChartDisplay::updateLookupTable()
{
  bool use_points = pqSMAdaptor::getElementProperty(
    this->getProxy()->GetProperty("XAxisUsePoints")).toBool();

  vtkDataArray* xarray  = this->getXArray();
  if (!xarray)
    {
    qDebug() << "Cannot set up lookup table, no X array.";
    return;
    }

  pqScalarsToColors* lut;
  // Now set up default lookup table.
  if (use_points || !xarray->GetName())
    {
    lut = this->setLookupTable("unnamedArray"); 
    }
  else
    {
    lut = this->setLookupTable(xarray->GetName());
    }
  if (lut)
    {
    // reset range of LUT respecting the range locks.
    double range[2];
    xarray->GetRange(range);
    lut->setWholeScalarRange(range[0], range[1]);
    }
}

//-----------------------------------------------------------------------------
vtkRectilinearGrid* pqBarChartDisplay::getClientSideData() const
{
  vtkSMGenericViewDisplayProxy* proxy = 
    vtkSMGenericViewDisplayProxy::SafeDownCast(this->getProxy());
  if (proxy)
    {
    return vtkRectilinearGrid::SafeDownCast(proxy->GetOutput());
    }
  return 0;
}

//-----------------------------------------------------------------------------
vtkDataArray* pqBarChartDisplay::getXArray()
{
  vtkSMProxy* proxy = this->getProxy();
  vtkRectilinearGrid* data = this->getClientSideData();
  if (!data || !proxy)
    {
    return 0;
    }

  bool use_points = pqSMAdaptor::getElementProperty(
    proxy->GetProperty("XAxisUsePoints")).toBool();
  if (use_points)
    {
    int component = pqSMAdaptor::getElementProperty(
      proxy->GetProperty("XAxisPointComponent")).toInt();
    switch (component)
      {
    case 0:
      return data->GetXCoordinates();

    case 1:
      return data->GetYCoordinates();

    case 2:
      return data->GetZCoordinates();
      }
    }
  else
    {
    QString xarrayName = pqSMAdaptor::getElementProperty(
      proxy->GetProperty("XArrayName")).toString();
    return data->GetPointData()->GetArray(xarrayName.toAscii().data());
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkDataArray* pqBarChartDisplay::getYArray()
{
  vtkSMProxy* proxy = this->getProxy();
  vtkRectilinearGrid* data = this->getClientSideData();
  if (!data || !proxy)
    {
    return 0;
    }
  QString yarrayName = pqSMAdaptor::getElementProperty(
    proxy->GetProperty("YArrayName")).toString();
  return data->GetCellData()->GetArray(yarrayName.toAscii().data());
}
