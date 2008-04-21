/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMSelectionHelper.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSelectionHelper.h"

#include "vtkClientServerStream.h"
#include "vtkCollection.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIterator.h"
#include "vtkInformationStringKey.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVDataInformation.h"
#include "vtkPVSelectionInformation.h"
#include "vtkSelection.h"
#include "vtkSelectionSerializer.h"
#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkUnsignedIntArray.h"

#include <vtkstd/vector>
#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkSMSelectionHelper);
vtkCxxRevisionMacro(vtkSMSelectionHelper, "$Revision: 1.14 $");

//-----------------------------------------------------------------------------
void vtkSMSelectionHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkSMSelectionHelper::SendSelection(vtkSelection* sel, vtkSMProxy* proxy)
{
  vtkProcessModule* processModule = vtkProcessModule::GetProcessModule();

  vtksys_ios::ostringstream res;
  vtkSelectionSerializer::PrintXML(res, vtkIndent(), 1, sel);
  vtkClientServerStream stream;
  vtkClientServerID parserID =
    processModule->NewStreamObject("vtkSelectionSerializer", stream);
  stream << vtkClientServerStream::Invoke
         << parserID << "Parse" << res.str().c_str() << proxy->GetID()
         << vtkClientServerStream::End;
  processModule->DeleteStreamObject(parserID, stream);

  processModule->SendStream(proxy->GetConnectionID(), 
    proxy->GetServers(), 
    stream);
}

//-----------------------------------------------------------------------------
void vtkSMSelectionHelper::ConvertSurfaceSelectionToVolumeSelection(
  vtkIdType connectionID,
  vtkSelection* input,
  vtkSelection* output)
{
  vtkSMSelectionHelper::ConvertSurfaceSelectionToVolumeSelectionInternal(
    connectionID, input, output, 0);
}

//-----------------------------------------------------------------------------
// Don't think this method is used anymore.
void vtkSMSelectionHelper::ConvertSurfaceSelectionToGlobalIDVolumeSelection(
  vtkIdType connectionID,
  vtkSelection* input, vtkSelection* output)
{
  vtkSMSelectionHelper::ConvertSurfaceSelectionToVolumeSelectionInternal(
    connectionID, input, output, 1);
}

//-----------------------------------------------------------------------------
void vtkSMSelectionHelper::ConvertSurfaceSelectionToVolumeSelectionInternal(
  vtkIdType connectionID,
  vtkSelection* input,
  vtkSelection* output,
  int global_ids)
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkProcessModule* processModule = vtkProcessModule::GetProcessModule();

  // Convert the selection of geometry cells to surface cells of the actual
  // data by:

  // * First sending the selection to the server
  vtkSMProxy* selectionP = pxm->NewProxy("selection_helpers", "Selection");
  selectionP->SetServers(vtkProcessModule::DATA_SERVER);
  selectionP->SetConnectionID(connectionID);
  selectionP->UpdateVTKObjects();
  vtkSMSelectionHelper::SendSelection(input, selectionP);

  // * Then converting the selection using a helper
  vtkSMProxy* volumeSelectionP = 
    pxm->NewProxy("selection_helpers", "Selection");
  volumeSelectionP->SetServers(vtkProcessModule::DATA_SERVER);
  volumeSelectionP->SetConnectionID(connectionID);
  volumeSelectionP->UpdateVTKObjects();
  vtkClientServerStream stream;
  vtkClientServerID converterID =
    processModule->NewStreamObject("vtkSelectionConverter", stream);
  stream << vtkClientServerStream::Invoke
         << converterID 
         << "Convert" 
         << selectionP->GetID() 
         << volumeSelectionP->GetID()
         << global_ids
         << vtkClientServerStream::End;
  processModule->DeleteStreamObject(converterID, stream);
  processModule->SendStream(connectionID, 
                            vtkProcessModule::DATA_SERVER, 
                            stream);

  // * And finally gathering the information back
  vtkPVSelectionInformation* selInfo = vtkPVSelectionInformation::New();
  processModule->GatherInformation(connectionID,
                                   vtkProcessModule::DATA_SERVER, 
                                   selInfo, 
                                   volumeSelectionP->GetID());

  output->ShallowCopy(selInfo->GetSelection());

  selInfo->Delete();
  volumeSelectionP->Delete();
  selectionP->Delete();
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMSelectionHelper::NewSelectionSourceFromSelectionInternal(
  vtkIdType connectionID, 
  vtkSelection* selection,
  vtkSMProxy* selSource /*=NULL*/)
{
  if (!selection)
    {
    return 0;
    }

  if (selection->GetNumberOfChildren()>0)
    {
    vtkGenericWarningMacro("Selection trees are not supported.");
    return selSource;
    }

  vtkSMProxy* originalSelSource = selSource;

  vtkInformation* selProperties = selection->GetProperties();
  int contentType = selection->GetContentType();

  // Determine the type of selection source proxy to create that will
  // generate the a vtkSelection same the "selection" instance passed as an
  // argument.
  const char* proxyname = 0;
  bool use_composite = false;
  bool use_hierarchical = false;
  switch (contentType)
    {
  case -1:
    // ContentType is not defined. Empty selection.
    return 0;

  case vtkSelection::FRUSTUM:
    proxyname = "FrustumSelectionSource";
    break;

  case vtkSelection::INDICES:
    // we need to choose between IDSelectionSource,
    // CompositeDataIDSelectionSource and HierarchicalDataIDSelectionSource.
    proxyname = "IDSelectionSource";
    if (selProperties->Has(vtkSelection::COMPOSITE_INDEX()))
      {
      proxyname = "CompositeDataIDSelectionSource";
      use_composite = true;
      }
    else if (selProperties->Has(vtkSelection::HIERARCHICAL_LEVEL()) && 
       selProperties->Has(vtkSelection::HIERARCHICAL_INDEX()))
      {
      proxyname = "HierarchicalDataIDSelectionSource";
      use_hierarchical = true;
      use_composite = false;
      }
    break;

  case vtkSelection::GLOBALIDS:
    proxyname = "GlobalIDSelectionSource";
    break;

  case vtkSelection::BLOCKS:
    proxyname = "BlockSelectionSource";
    break;

  default:
    vtkGenericWarningMacro("Unhandled ContentType: " << contentType);
    return selSource;
    }

  if (selSource && strcmp(selSource->GetXMLName(), proxyname) != 0)
    {
    vtkGenericWarningMacro("A composite selection has different types of selections."
      "This is not supported.");
    return selSource;
    }
  
  if (!selSource)
    {
    // If selSource is not present we need to create a new one. The type of
    // proxy we instantiate depends on the type of the vtkSelection.
    vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
    selSource = pxm->NewProxy("sources", proxyname);
    selSource->SetConnectionID(connectionID);
    selSource->SetServers(vtkProcessModule::DATA_SERVER);
    }


  // Set some common property values using the state of the vtkSelection.
  if (selProperties->Has(vtkSelection::FIELD_TYPE()))
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      selSource->GetProperty("FieldType"));
    ivp->SetElement(0, selProperties->Get(vtkSelection::FIELD_TYPE()));
    }

  if (selProperties->Has(vtkSelection::CONTAINING_CELLS()))
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      selSource->GetProperty("ContainingCells"));
    ivp->SetElement(0, selProperties->Get(vtkSelection::CONTAINING_CELLS()));
    }

  if (selProperties->Has(vtkSelection::INVERSE()))
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      selSource->GetProperty("InsideOut"));
    ivp->SetElement(0, selProperties->Get(vtkSelection::INVERSE()));
    }

  if (contentType == vtkSelection::FRUSTUM)
    {
    // Set the selection ids, which is the frustum vertex.
    vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      selSource->GetProperty("Frustum"));

    vtkDoubleArray* verts = vtkDoubleArray::SafeDownCast(
      selection->GetSelectionList());
    dvp->SetElements(verts->GetPointer(0));
    }
  else if (contentType == vtkSelection::GLOBALIDS)
    {
    vtkSMIdTypeVectorProperty* ids = vtkSMIdTypeVectorProperty::SafeDownCast(
      selSource->GetProperty("IDs"));
    if (!originalSelSource)
      {
      ids->SetNumberOfElements(0);
      }
    unsigned int curValues = ids->GetNumberOfElements();
    vtkIdTypeArray* idList = vtkIdTypeArray::SafeDownCast(
      selection->GetSelectionList());
    if (idList)
      {
      vtkIdType numIDs = idList->GetNumberOfTuples();
      ids->SetNumberOfElements(curValues+numIDs);
      for (vtkIdType cc=0; cc < numIDs; cc++)
        {
        ids->SetElement(curValues+cc, idList->GetValue(cc));
        }
      }
    }
  else if (contentType == vtkSelection::BLOCKS)
    {
    vtkSMIdTypeVectorProperty* blocks = vtkSMIdTypeVectorProperty::SafeDownCast(
      selSource->GetProperty("Blocks"));
    if (!originalSelSource)
      {
      blocks->SetNumberOfElements(0);
      }
    unsigned int curValues = blocks->GetNumberOfElements();
    vtkUnsignedIntArray* idList = vtkUnsignedIntArray::SafeDownCast(
      selection->GetSelectionList());
    if (idList)
      {
      vtkIdType numIDs = idList->GetNumberOfTuples();
      blocks->SetNumberOfElements(curValues+numIDs);
      for (vtkIdType cc=0; cc < numIDs; cc++)
        {
        blocks->SetElement(curValues+cc, idList->GetValue(cc));
        }
      }
    }
  else if (contentType == vtkSelection::INDICES)
    {
    vtkIdType procID = -1;
    if (selProperties->Has(vtkSelection::PROCESS_ID()))
      {
      procID = selProperties->Get(vtkSelection::PROCESS_ID());
      }

    // Add the selection proc ids and cell ids to the IDs property.
    vtkSMIdTypeVectorProperty* ids = vtkSMIdTypeVectorProperty::SafeDownCast(
      selSource->GetProperty("IDs"));
    if (!originalSelSource)
      {
      // remove default values set by the XML if we created a brand new proxy.
      ids->SetNumberOfElements(0);
      }
    unsigned int curValues = ids->GetNumberOfElements();
    vtkIdTypeArray* idList = vtkIdTypeArray::SafeDownCast(
      selection->GetSelectionList());
    if (idList)
      {
      vtkIdType numIDs = idList->GetNumberOfTuples();
      if (!use_composite && !use_hierarchical)
        {
        ids->SetNumberOfElements(curValues+numIDs*2);
        for (vtkIdType cc=0; cc < numIDs; cc++)
          {
          ids->SetElement(curValues+2*cc, procID);
          ids->SetElement(curValues+2*cc+1, idList->GetValue(cc));
          }
        }
      else if (use_composite)
        {
        vtkIdType composite_index = 0;
        if (selProperties->Has(vtkSelection::COMPOSITE_INDEX()))
          {
          composite_index = selProperties->Get(vtkSelection::COMPOSITE_INDEX());
          }

        ids->SetNumberOfElements(curValues+numIDs*3);
        for (vtkIdType cc=0; cc < numIDs; cc++)
          {
          ids->SetElement(curValues+3*cc, composite_index);
          ids->SetElement(curValues+3*cc+1, procID);
          ids->SetElement(curValues+3*cc+2, idList->GetValue(cc));
          }
        }
      else if (use_hierarchical)
        {
        vtkIdType level = selProperties->Get(vtkSelection::HIERARCHICAL_LEVEL());
        vtkIdType dsIndex = selProperties->Get(vtkSelection::HIERARCHICAL_INDEX());
        ids->SetNumberOfElements(curValues+numIDs*3);
        for (vtkIdType cc=0; cc < numIDs; cc++)
          {
          ids->SetElement(curValues+3*cc, level);
          ids->SetElement(curValues+3*cc+1, dsIndex);
          ids->SetElement(curValues+3*cc+2, idList->GetValue(cc));
          }
        }
      }
    }

  return selSource;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMSelectionHelper::NewSelectionSourceFromSelection(
  vtkIdType connectionID,
  vtkSelection* selection)
{
  vtkSMProxy* selSource= 0;
  if (selection->GetNumberOfChildren() == 0)
    {
    selSource = vtkSMSelectionHelper::NewSelectionSourceFromSelectionInternal(
      connectionID, selection, NULL);
    }
  else
    {
    unsigned int numChildren = selection->GetNumberOfChildren(); 
    for (unsigned int cc=0; cc < numChildren; cc++)
      {
      vtkSelection* child = selection->GetChild(cc);
      selSource = vtkSMSelectionHelper::NewSelectionSourceFromSelectionInternal(
        connectionID, child, selSource);
      }
    }
  if (selSource)
    {
    selSource->UpdateVTKObjects();
    }
  return selSource;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMSelectionHelper::ConvertSelection(int outputType,
  vtkSMProxy* selectionSourceProxy,
  vtkSMSourceProxy* dataSource, int dataPort)
{
  const char* inproxyname = selectionSourceProxy? 
    selectionSourceProxy->GetXMLName() : 0;
  const char* outproxyname = 0;
  switch (outputType)
    {
  case vtkSelection::GLOBALIDS:
    outproxyname = "GlobalIDSelectionSource";
    break;

  case vtkSelection::FRUSTUM:
    outproxyname = "FrustumSelectionSource";
    break;

  case vtkSelection::LOCATIONS:
    outproxyname = "LocationSelectionSource";
    break;

  case vtkSelection::THRESHOLDS:
    outproxyname = "ThresholdSelectionSource";
    break;

  case vtkSelection::BLOCKS:
    outproxyname = "BlockSelectionSource";
    break;

  case vtkSelection::INDICES:
      {
      vtkPVDataInformation* di = dataSource->GetDataInformation(dataPort, true);
      outproxyname = "IDSelectionSource";
      if (di->GetCompositeDataClassName())
        {
        if (strcmp(di->GetCompositeDataClassName(), "vtkHierarchicalBoxDataSet")==0)
          {
          outproxyname = "HierarchicalDataIDSelectionSource";
          }
        else
          {
          outproxyname = "CompositeDataIDSelectionSource";
          }
        }
      }
    break;

  default:
    vtkGenericWarningMacro("Cannot convert to type : " << outputType);
    return 0;
    }

  if (selectionSourceProxy && strcmp(inproxyname, outproxyname) == 0)
    {
    // No conversion needed.
    selectionSourceProxy->Register(0);
    return selectionSourceProxy;
    }

  if (outputType == vtkSelection::INDICES && selectionSourceProxy &&
    strcmp(inproxyname, "GlobalIDSelectionSource") == 0)
    {
    vtkSMVectorProperty* ids = vtkSMVectorProperty::SafeDownCast(
      selectionSourceProxy->GetProperty("IDs"));
    if (ids->GetNumberOfElements() > 0)
      {
      // convert from global IDs to indices.
      return vtkSMSelectionHelper::ConvertInternal(
        vtkSMSourceProxy::SafeDownCast(selectionSourceProxy),
        dataSource, dataPort, vtkSelection::INDICES);
      }
    }
  else if (outputType == vtkSelection::GLOBALIDS && selectionSourceProxy && (
      strcmp(inproxyname, "IDSelectionSource") == 0 ||
      strcmp(inproxyname, "HierarchicalDataIDSelectionSource") == 0||
      strcmp(inproxyname, "CompositeDataIDSelectionSource")==0))
    {
    vtkSMVectorProperty* ids = vtkSMVectorProperty::SafeDownCast(
      selectionSourceProxy->GetProperty("IDs"));
    if (ids->GetNumberOfElements() > 0)
      {
      // convert from ID seelction to global IDs.
      return vtkSMSelectionHelper::ConvertInternal(
        vtkSMSourceProxy::SafeDownCast(selectionSourceProxy),
        dataSource, dataPort, vtkSelection::GLOBALIDS);
      }
    }
  else if (outputType == vtkSelection::BLOCKS && selectionSourceProxy &&
    (strcmp(inproxyname, "GlobalIDSelectionSource") == 0 ||
     strcmp(inproxyname, "HierarchicalDataIDSelectionSource") == 0||
     strcmp(inproxyname, "CompositeDataIDSelectionSource")==0))
    {
    return vtkSMSelectionHelper::ConvertInternal(
      vtkSMSourceProxy::SafeDownCast(selectionSourceProxy),
      dataSource, dataPort, vtkSelection::BLOCKS);
    }

  // Conversion not possible, so simply create a new proxy of the requested
  // output type with some empty defaults.
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* outSource = pxm->NewProxy("sources", outproxyname);
  if (!outSource)
    {
    return outSource;
    }

  // Note that outSource->ConnectionID and outSource->Servers are not yet set.
  if (vtkSMVectorProperty* vp = vtkSMVectorProperty::SafeDownCast(
      outSource->GetProperty("IDs")))
    {
    // remove default ID values.
    vp->SetNumberOfElements(0);
    }

  if (selectionSourceProxy)
    {
    outSource->SetServers(selectionSourceProxy->GetServers());
    outSource->SetConnectionID(selectionSourceProxy->GetConnectionID());

    // try to copy as many properties from the old-source to the new one.
    outSource->GetProperty("ContainingCells")->Copy(
      selectionSourceProxy->GetProperty("ContainingCells"));
    outSource->GetProperty("FieldType")->Copy(
      selectionSourceProxy->GetProperty("FieldType"));
    outSource->GetProperty("InsideOut")->Copy(
      selectionSourceProxy->GetProperty("InsideOut"));
    outSource->UpdateVTKObjects();
    }
  return outSource;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMSelectionHelper::ConvertInternal(
  vtkSMSourceProxy* inSource, vtkSMSourceProxy* dataSource,
  int dataPort, int outputType)
{
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  // * Update all inputs.
  inSource->UpdatePipeline();
  dataSource->UpdatePipeline();

  // * Filter that converts selections.
  vtkSMSourceProxy* convertor = vtkSMSourceProxy::SafeDownCast(
    pxm->NewProxy("filters", "ConvertSelection"));
  convertor->SetConnectionID(inSource->GetConnectionID());
  convertor->SetServers(inSource->GetServers());

  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(
    convertor->GetProperty("Input"));
  ip->AddInputConnection(inSource, 0);

  ip = vtkSMInputProperty::SafeDownCast(convertor->GetProperty("DataInput"));
  ip->AddInputConnection(dataSource, dataPort);

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    convertor->GetProperty("OutputType"));
  ivp->SetElement(0, outputType);
  convertor->UpdateVTKObjects();

  // * Request conversion.
  convertor->UpdatePipeline();

  // * And finally gathering the information back
  vtkPVSelectionInformation* selInfo = vtkPVSelectionInformation::New();
  pm->GatherInformation(convertor->GetConnectionID(),
                        convertor->GetServers(),
                        selInfo,
                        convertor->GetID());


  vtkSMProxy* outSource = vtkSMSelectionHelper::NewSelectionSourceFromSelection(
    inSource->GetConnectionID(), selInfo->GetSelection());


  // cleanup.
  convertor->Delete();
  selInfo->Delete();

  return outSource;
}
