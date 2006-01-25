// -*- c++ -*-

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqParts.h"

#include <vtkSMDataObjectDisplayProxy.h>
#include <vtkSMDisplayProxy.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkPVDataSetAttributesInformation.h>
#include <vtkPVGeometryInformation.h>
#include <vtkPVArrayInformation.h>
#include <vtkSMLookupTableProxy.h>
#include <vtkSMProxyManager.h>
#include <vtkProcessModule.h>

#include "pqPipelineData.h"
#include "pqSMAdaptor.h"
#include "pqPipelineObject.h"

vtkSMDisplayProxy* pqAddPart(vtkSMRenderModuleProxy* rm, vtkSMSourceProxy* Part)
{
  // without this, you will get runtime errors from the part display
  // (connected below). this should be fixed
  Part->CreateParts();

  // Create part display.
  vtkSMDisplayProxy *partdisplay = rm->CreateDisplayProxy();

  // Set the part as input to the part display.
  vtkSMProxyProperty *pp
    = vtkSMProxyProperty::SafeDownCast(partdisplay->GetProperty("Input"));
  pp->RemoveAllProxies();
  pp->AddProxy(Part);

  vtkSMDataObjectDisplayProxy *dod
    = vtkSMDataObjectDisplayProxy::SafeDownCast(partdisplay);
  if (dod)
    {
    dod->SetRepresentationCM(vtkSMDataObjectDisplayProxy::SURFACE);
    }

  partdisplay->UpdateVTKObjects();

  // Add the part display to the render module.
  pp = vtkSMProxyProperty::SafeDownCast(rm->GetProperty("Displays"));
  pp->AddProxy(partdisplay);
  
  rm->UpdateVTKObjects();

  // set default colors for display
  pqColorPart(partdisplay);

  // Allow the render module proxy to maintain the part display.
//  partdisplay->Delete();

  return partdisplay;
}

void pqRemovePart(vtkSMRenderModuleProxy* rm, vtkSMDisplayProxy* Part)
{
  vtkSMProxyProperty *pp
    = vtkSMProxyProperty::SafeDownCast(rm->GetProperty("Displays"));
  pp->RemoveProxy(Part);
  rm->UpdateVTKObjects();
}


static void pqGetColorArray(
  vtkPVDataSetAttributesInformation* attrInfo,
  vtkPVDataSetAttributesInformation* inAttrInfo,
  vtkPVArrayInformation*& arrayInfo)
{  
  arrayInfo = NULL;

  // Check for new point scalars.
  vtkPVArrayInformation* tmp =
    attrInfo->GetAttributeInformation(vtkDataSetAttributes::SCALARS);
  vtkPVArrayInformation* inArrayInfo = 0;
  if (tmp)
    {
    if (inAttrInfo)
      {
      inArrayInfo = inAttrInfo->GetAttributeInformation(
        vtkDataSetAttributes::SCALARS);
      }
    if (inArrayInfo == 0 ||
      strcmp(tmp->GetName(),inArrayInfo->GetName()) != 0)
      { 
      // No input or different scalars: use the new scalars.
      arrayInfo = tmp;
      }
    }
}


/// color the part to its default color
void pqColorPart(vtkSMDisplayProxy* Part)
{
  // if the source created a new point scalar, use it
  // else if the source created a new cell scalar, use it
  // else if the input color by array exists in this source, use it
  // else color by property
  
  vtkPVDataInformation* inGeomInfo = 0;
  vtkPVDataInformation* geomInfo = 0;
  vtkPVDataSetAttributesInformation* inAttrInfo = 0;
  vtkPVDataSetAttributesInformation* attrInfo;
  vtkPVArrayInformation* arrayInfo;
  vtkSMDisplayProxy* dproxy;

  dproxy = Part;
  if (dproxy)
    {
    geomInfo = dproxy->GetGeometryInformation();
    }
    
#if 0
  pqPipelineObject* input = 0;
  pqPipelineData* pipeline = pqPipelineData::instance();
  pqPipelineObject* pqpart = pipeline->getObjectFor(Part);  // this doesn't work with compound proxies
  input = pqpart->GetInput(0);

  if (input)
    {
    dproxy = input->GetDisplayProxy();
    if (dproxy)
      {
      inGeomInfo = dproxy->GetGeometryInformation();
      }
    }
#endif
  
  // Look for a new point array.
  // I do not think the logic is exactly as describerd in this methods
  // comment.  I believe this method only looks at "Scalars".
  attrInfo = geomInfo->GetPointDataInformation();
  if (inGeomInfo)
    {
    inAttrInfo = inGeomInfo->GetPointDataInformation();
    }
  else
    {
    inAttrInfo = 0;
    }
  pqGetColorArray(attrInfo, inAttrInfo, arrayInfo);
  if(arrayInfo)
    {
    pqColorPart(Part, arrayInfo->GetName(), vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA);
    return;
    }
    
  // Check for new cell scalars.
  attrInfo = geomInfo->GetCellDataInformation();
  if (inGeomInfo)
    {
    inAttrInfo = inGeomInfo->GetCellDataInformation();
    }
  else
    {
    inAttrInfo = 0;
    }
  pqGetColorArray(attrInfo, inAttrInfo, arrayInfo);
  if(arrayInfo)
    {
    pqColorPart(Part, arrayInfo->GetName(), vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA);
    return;
    }
    
#if 0
  // Inherit property color from input.
  if (input)
    {
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      input->GetDisplayProxy()->GetProperty("Color"));
    if (!dvp)
      {
      vtkErrorMacro("Failed to find property Color on input->DisplayProxy.");
      return;
      }
    double rgb[3] = { 1, 1, 1};
    input->GetDisplayProxy()->GetColorCM(rgb);
    this->DisplayProxy->SetColorCM(rgb);
    this->DisplayProxy->UpdateVTKObjects();
    } 
#endif

  if (geomInfo)
    {
    // Check for scalars in geometry
    attrInfo = geomInfo->GetPointDataInformation();
    pqGetColorArray(attrInfo, inAttrInfo, arrayInfo);
    if(arrayInfo)
      {
      pqColorPart(Part, arrayInfo->GetName(), vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA);
      return;
      }
    }

  if (geomInfo)
    {
    // Check for scalars in geometry
    attrInfo = geomInfo->GetCellDataInformation();
    pqGetColorArray(attrInfo, inAttrInfo, arrayInfo);
    if(arrayInfo)
      {
      pqColorPart(Part, arrayInfo->GetName(), vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA);
      return;
      }
    }

#if 0
  // Try to use the same array selected by the input.
  if (input)
    {
    colorMap = input->GetPVColorMap();
    int colorField = -1;
    if (colorMap)
      {
      colorField = input->GetDisplayProxy()->GetScalarModeCM();

      // Find the array in our info.
      switch (colorField)
        {
      case vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA:
          attrInfo = geomInfo->GetPointDataInformation();
          arrayInfo = attrInfo->GetArrayInformation(colorMap->GetArrayName());
          if (arrayInfo && colorMap->MatchArrayName(arrayInfo->GetName(),
                                       arrayInfo->GetNumberOfComponents()))
            {  
            this->ColorByArray(
              colorMap, vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA);
            return;
            }
          break;
        case vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA:
          attrInfo = geomInfo->GetCellDataInformation();
          arrayInfo = attrInfo->GetArrayInformation(colorMap->GetArrayName());
          if (arrayInfo && colorMap->MatchArrayName(arrayInfo->GetName(),
                                       arrayInfo->GetNumberOfComponents()))
            {  
            this->ColorByArray(
              colorMap, vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA);
            return;
            }
          break;
        default:
          vtkErrorMacro("Bad attribute.");
          return;
        }

      }
    }
#endif

  // Color by property.
  pqColorPart(Part, NULL, 0);
}

/// color the part by a specific field, if fieldname is NULL, colors by actor color
void pqColorPart(vtkSMDisplayProxy* Part, const char* fieldname, int fieldtype)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(Part->GetProperty("LookupTable"));
  pp->RemoveAllProxies();
  
  if(fieldname == 0)
    {
    pqSMAdaptor::instance()->setProperty(Part->GetProperty("ScalarVisibility"), 0);
    }
  else
    {
    // create lut
    vtkSMLookupTableProxy* lut = vtkSMLookupTableProxy::SafeDownCast(
      vtkSMObject::GetProxyManager()->NewProxy("lookup_tables", "LookupTable"));
    lut->SetServers(vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
    pp = vtkSMProxyProperty::SafeDownCast(Part->GetProperty("LookupTable"));
    pp->AddProxy(lut);
    
    pqSMAdaptor::instance()->setProperty(Part->GetProperty("ScalarVisibility"), 1);
    pqSMAdaptor::instance()->setProperty(Part->GetProperty("ScalarMode"), fieldtype);

    vtkPVArrayInformation* ai;

    if(fieldtype == vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA)
      {
      vtkPVDataInformation* geomInfo = Part->GetGeometryInformation();
      ai = geomInfo->GetCellDataInformation()->GetArrayInformation(fieldname);
      }
    else
      {
      vtkPVDataInformation* geomInfo = Part->GetGeometryInformation();
      ai = geomInfo->GetPointDataInformation()->GetArrayInformation(fieldname);
      }
    
    // array couldn't be found, look for it on the reader
    // TODO: this support should be moved into the server manager and/or VTK
    if(!ai)
      {
      pp = vtkSMProxyProperty::SafeDownCast(Part->GetProperty("Input"));
      vtkSMProxy* reader = pp->GetProxy(0);
      while((pp = vtkSMProxyProperty::SafeDownCast(reader->GetProperty("Input"))))
        reader = pp->GetProxy(0);
      QList<QVariant> property;
      property += fieldname;
      property += 1;
      if(fieldtype == vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA)
        {
        pqSMAdaptor::instance()->setProperty(reader->GetProperty("CellArrayStatus"), 0, property);
        reader->UpdateVTKObjects();
        vtkPVDataInformation* geomInfo = Part->GetGeometryInformation();
        ai = geomInfo->GetCellDataInformation()->GetArrayInformation(fieldname);
        }
      else
        {
        pqSMAdaptor::instance()->setProperty(reader->GetProperty("PointArrayStatus"), 0, property);
        reader->UpdateVTKObjects();
        vtkPVDataInformation* geomInfo = Part->GetGeometryInformation();
        ai = geomInfo->GetPointDataInformation()->GetArrayInformation(fieldname);
        }
      }
    double range[2] = {0,1};
    if(ai)
      {
      ai->GetComponentRange(0, range);
      }
    QList<QVariant> tmp;
    tmp += range[0];
    tmp += range[1];
    pqSMAdaptor::instance()->setProperty(lut->GetProperty("ScalarRange"), tmp);
    pqSMAdaptor::instance()->setProperty(Part->GetProperty("ColorArray"), fieldname);
    lut->UpdateVTKObjects();
    }

  Part->UpdateVTKObjects();
}

