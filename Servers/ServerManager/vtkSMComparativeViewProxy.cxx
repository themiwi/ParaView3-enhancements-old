/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMComparativeViewProxy.cxx,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMComparativeViewProxy.h"

#include "vtkCollection.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxyLink.h"
#include "vtkSMProxyManager.h"
#include "vtkSMPVAnimationSceneProxy.h"
#include "vtkSMRepresentationProxy.h"

#include <vtkstd/vector>
#include <vtkstd/map>

class vtkSMComparativeViewProxy::vtkInternal
{
public:
  struct RepresentationData
    {
    typedef vtkstd::map<vtkSMViewProxy*, vtkSmartPointer<vtkSMRepresentationProxy> > MapOfViewToRepr;
    MapOfViewToRepr Clones; // The clone representations created for the representations (with key  
                            // being the view in which that representation clone
                            // exists.
    vtkSmartPointer<vtkSMProxyLink> Link;
    };

  typedef vtkstd::vector<vtkSmartPointer<vtkSMViewProxy> > VectorOfViews;
  VectorOfViews Views;


  typedef vtkstd::map<vtkSMRepresentationProxy*, RepresentationData> MapOfReprClones;

  MapOfReprClones RepresentationClones;

  vtkSmartPointer<vtkSMProxyLink> ViewLink;

  vtkInternal()
    {
    this->ViewLink = vtkSmartPointer<vtkSMProxyLink>::New();
    }

  unsigned int ActiveIndexX;
  unsigned int ActiveIndexY;
};

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkSMComparativeViewProxy);
vtkCxxRevisionMacro(vtkSMComparativeViewProxy, "$Revision: 1.2 $");
vtkCxxSetObjectMacro(vtkSMComparativeViewProxy, AnimationSceneX, vtkSMPVAnimationSceneProxy);
vtkCxxSetObjectMacro(vtkSMComparativeViewProxy, AnimationSceneY, vtkSMPVAnimationSceneProxy);
//----------------------------------------------------------------------------
vtkSMComparativeViewProxy::vtkSMComparativeViewProxy()
{
  this->Internal = new vtkInternal();
  this->Dimensions[0] = 0;
  this->Dimensions[1] = 0;
  this->AnimationSceneX = 0;
  this->AnimationSceneY = 0;

  vtkMemberFunctionCommand<vtkSMComparativeViewProxy>* fsO = 
    vtkMemberFunctionCommand<vtkSMComparativeViewProxy>::New();
  fsO->SetCallback(*this, &vtkSMComparativeViewProxy::FilmStripTick);
  this->FilmStripObserver = fsO;
}

//----------------------------------------------------------------------------
vtkSMComparativeViewProxy::~vtkSMComparativeViewProxy()
{
  this->SetAnimationSceneX(0);
  this->SetAnimationSceneY(0);
  this->FilmStripObserver->Delete();
  delete this->Internal;
}

//----------------------------------------------------------------------------
bool vtkSMComparativeViewProxy::BeginCreateVTKObjects()
{
  vtkSMViewProxy* rootView = vtkSMViewProxy::SafeDownCast(
    this->GetSubProxy("RootView"));
  if (!rootView)
    {
    vtkErrorMacro("Subproxy \"Root\" must be defined in the xml configuration.");
    return false;
    }

  this->Dimensions[0] = 1;
  this->Dimensions[1] = 1;

  // Root view is the first view in the views list.
  this->Internal->Views.push_back(rootView);

  this->Internal->ViewLink->AddLinkedProxy(rootView, vtkSMLink::INPUT);

  // Every view keeps their own representations.
  this->Internal->ViewLink->AddException("Representations");

  // This view computes view size/view position for each view based on the
  // layout.
  this->Internal->ViewLink->AddException("ViewSize");
  this->Internal->ViewLink->AddException("ViewPosition");
  return this->Superclass::BeginCreateVTKObjects();
}

//----------------------------------------------------------------------------
void vtkSMComparativeViewProxy::Build(int dx, int dy)
{
  // Ensure objects are created before building.
  this->CreateVTKObjects();

  if (dx == 0 || dy == 0)
    {
    vtkErrorMacro("Dimensions cannot be 0.");
    return;
    }

  int numViews = dx * dy;
  int cc;

  // Remove extra view modules.
  for (cc=this->Internal->Views.size()-1; cc >= numViews; cc--)
    {
    this->RemoveView(this->Internal->Views[cc]);
    }

  // Add view modules, if not enough.
  for (cc=this->Internal->Views.size(); cc < numViews; cc++)
    {
    this->AddNewView();
    }

  this->Dimensions[0] = dx;
  this->Dimensions[1] = dy;

  // TODO: Update ViewSize ViewPosition for all internal views.
}

//----------------------------------------------------------------------------
vtkSMViewProxy* vtkSMComparativeViewProxy::GetRootView()
{
  return this->Internal->Views[0];
}

//----------------------------------------------------------------------------
void vtkSMComparativeViewProxy::AddNewView()
{
  vtkSMViewProxy* rootView = this->GetRootView();

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMViewProxy* newView = vtkSMViewProxy::SafeDownCast(
    pxm->NewProxy(rootView->GetXMLGroup(), rootView->GetXMLName()));
  if (!newView)
    {
    vtkErrorMacro("Failed to create internal view proxy. Comparative visualization "
      "view cannot work.");
    return;
    }
 

  this->Internal->Views.push_back(newView);
  this->Internal->ViewLink->AddLinkedProxy(newView, vtkSMLink::OUTPUT);
  newView->Delete();

  // Ensure that views always use cache.
  newView->SetUseCache(true);
  newView->SetCacheTime(0); // cache time value is merely is place holder. 
                            // All views cache only 1 time, hence this number is
                            // immaterial.

  // Create clones for all currently added representation for the new view.
  vtkInternal::MapOfReprClones::iterator reprIter;
  for (reprIter = this->Internal->RepresentationClones.begin();
    reprIter != this->Internal->RepresentationClones.end(); ++reprIter)
    {
    vtkSMRepresentationProxy* repr = reprIter->first;
    vtkInternal::RepresentationData& data = reprIter->second;

    vtkSMRepresentationProxy* newRepr = vtkSMRepresentationProxy::SafeDownCast(
      pxm->NewProxy(repr->GetXMLGroup(), repr->GetXMLName()));
    newRepr->Copy(repr); // create a clone.
    newRepr->UpdateVTKObjects();

    data.Link->AddLinkedProxy(newRepr, vtkSMLink::OUTPUT);
    newView->AddRepresentation(newRepr);
      
    // Now update data structure to include this view/repr clone.
    data.Clones[newView] = newRepr;
    newRepr->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkSMComparativeViewProxy::RemoveView(vtkSMViewProxy* view)
{
  if (view == this->GetRootView())
    {
    vtkErrorMacro("Root view cannot be removed.");
    return;
    }

  // Remove all representations in this new.
  vtkInternal::MapOfReprClones::iterator reprIter;
  for (reprIter = this->Internal->RepresentationClones.begin();
    reprIter != this->Internal->RepresentationClones.end(); ++reprIter)
    {
    vtkInternal::RepresentationData& data = reprIter->second;
    vtkInternal::RepresentationData::MapOfViewToRepr::iterator cloneIter
      = data.Clones.find(view);
    if (cloneIter != data.Clones.end())
      {
      vtkSMRepresentationProxy* clone = cloneIter->second.GetPointer();
      view->RemoveRepresentation(clone);
      data.Link->RemoveLinkedProxy(clone);

      data.Clones.erase(cloneIter);
      }
    }

  this->Internal->ViewLink->RemoveLinkedProxy(view);
  vtkInternal::VectorOfViews::iterator iter;
  for (iter = this->Internal->Views.begin(); 
       iter != this->Internal->Views.end(); ++iter)
    {
    if (iter->GetPointer() == view)
      {
      this->Internal->Views.erase(iter);
      break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMComparativeViewProxy::AddRepresentation(vtkSMRepresentationProxy* repr)
{
  if (repr)
    {
    // Add repr to RootView. Create clones of repr and add them to all other
    // views.
    vtkSMViewProxy* rootView = this->GetRootView();
    rootView->AddRepresentation(repr);

    // We need to save information about the clones we create for this
    // representation so that we can clean them up later.
    vtkInternal::RepresentationData data;
    vtkSMProxyLink* reprLink = vtkSMProxyLink::New();
    data.Link.TakeReference(reprLink);

    reprLink->AddException("UpdateTime");
    reprLink->AddLinkedProxy(repr, vtkSMLink::INPUT);

    vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
    vtkInternal::VectorOfViews::iterator iter = this->Internal->Views.begin();
    iter++; // skip root view.
    for (; iter != this->Internal->Views.end(); ++iter)
      {
      vtkSMRepresentationProxy* newRepr = vtkSMRepresentationProxy::SafeDownCast(
        pxm->NewProxy(repr->GetXMLGroup(), repr->GetXMLName()));
      newRepr->Copy(repr); // create a clone.
      newRepr->UpdateVTKObjects();

      reprLink->AddLinkedProxy(newRepr, vtkSMLink::OUTPUT);
      iter->GetPointer()->AddRepresentation(newRepr);
      
      // Now update data structure to include this view/repr clone.
      data.Clones[iter->GetPointer()] = newRepr;
      newRepr->Delete();
      }

    this->Internal->RepresentationClones[repr] = data;
    }

  // Override superclass' AddRepresentation since  repr has already been added
  // to the rootView.
}

//----------------------------------------------------------------------------
void vtkSMComparativeViewProxy::RemoveRepresentation(vtkSMRepresentationProxy* repr)
{
  vtkInternal::MapOfReprClones::iterator reprDataIter
    = this->Internal->RepresentationClones.find(repr);
  if (!repr || reprDataIter == this->Internal->RepresentationClones.end())
    {
    // Nothing to do.
    return;
    }

  vtkInternal::RepresentationData& data = reprDataIter->second;

  // Remove all clones of this representation.
  vtkInternal::RepresentationData::MapOfViewToRepr::iterator viewReprIter;
  for (viewReprIter = data.Clones.begin(); viewReprIter != data.Clones.end(); ++viewReprIter)
    {
    vtkSMViewProxy* view = viewReprIter->first;
    vtkSMRepresentationProxy* clone = viewReprIter->second.GetPointer();
    if (view && clone)
      {
      view->RemoveRepresentation(clone);
      // No need to clean the clone from the proxy link since the link object
      // will be destroyed anyways.
      }
    }

  // This will destroy the repr proxy link as well.
  this->Internal->RepresentationClones.erase(reprDataIter);

  // Remove repr from RootView.
  vtkSMViewProxy* rootView = this->GetRootView();
  rootView->RemoveRepresentation(repr);


  // Override superclass' RemoveRepresentation since  repr was not added to this
  // view at all, we added it to (and removed from) the root view.
}

//----------------------------------------------------------------------------
void vtkSMComparativeViewProxy::RemoveAllRepresentations()
{
  vtkInternal::MapOfReprClones::iterator iter = 
    this->Internal->RepresentationClones.begin();
  while (iter != this->Internal->RepresentationClones.end())
    {
    vtkSMRepresentationProxy* repr = iter->first;
    this->RemoveRepresentation(repr);
    iter = this->Internal->RepresentationClones.begin();
    }
}

//----------------------------------------------------------------------------
void vtkSMComparativeViewProxy::StillRender()
{
  // Generate the CV if required.
  // For starters, we wont update the vis automatically, let the user call
  // UpdateComparativeVisualization explicitly.
  // this->UpdateComparativeVisualization();

  vtkInternal::VectorOfViews::iterator iter;
  for (iter = this->Internal->Views.begin(); 
       iter != this->Internal->Views.end(); ++iter)
    {
    iter->GetPointer()->StillRender();
    }
}

//----------------------------------------------------------------------------
void vtkSMComparativeViewProxy::InteractiveRender()
{
  vtkInternal::VectorOfViews::iterator iter;
  for (iter = this->Internal->Views.begin(); 
       iter != this->Internal->Views.end(); ++iter)
    {
    iter->GetPointer()->InteractiveRender();
    }
}

//----------------------------------------------------------------------------
vtkSMRepresentationProxy* vtkSMComparativeViewProxy::CreateDefaultRepresentation(
  vtkSMProxy* src)
{
  return this->GetRootView()->CreateDefaultRepresentation(src);
}

//----------------------------------------------------------------------------
void vtkSMComparativeViewProxy::UpdateVisualization()
{
  if (!this->AnimationSceneX && !this->AnimationSceneY)
    {
    // no comparative vis.
    return;
    }

  // Are we in generating a film-strip or a comparative vis?
  if (this->AnimationSceneX && this->AnimationSceneY)
    {
    this->UpdateComparativeVisualization();
    }
  else
    {
    this->UpdateFilmStripVisualization(
      this->AnimationSceneX? this->AnimationSceneX : this->AnimationSceneY);
    }
}

//----------------------------------------------------------------------------
void vtkSMComparativeViewProxy::UpdateFilmStripVisualization(
  vtkSMPVAnimationSceneProxy* scene)
{
  scene->SetPlayMode(vtkSMPVAnimationSceneProxy::SEQUENCE);
  scene->SetNumberOfFrames(this->Dimensions[0]*this->Dimensions[1]);
  scene->GoToFirst();
  scene->SetLoop(0);

  this->Internal->ActiveIndexX = 0;
  this->Internal->ActiveIndexY = 0;

  // Add observer to listen to all ticks.
  scene->AddObserver(vtkCommand::AnimationCueTickEvent, this->FilmStripObserver);
  scene->Play();
  scene->RemoveObserver(this->FilmStripObserver);
}

//----------------------------------------------------------------------------
void vtkSMComparativeViewProxy::UpdateComparativeVisualization()
{
}

//----------------------------------------------------------------------------
void vtkSMComparativeViewProxy::FilmStripTick()
{
  if (this->Internal->ActiveIndexX >= this->Internal->Views.size())
    {
    vtkErrorMacro("Internal error.");
    return;
    }

  vtkSMViewProxy* view = this->Internal->Views[this->Internal->ActiveIndexX];

  // Make the view cache the current setup. 
  view->StillRender();

  this->Internal->ActiveIndexX++;
}

//----------------------------------------------------------------------------
void vtkSMComparativeViewProxy::GetViews(vtkCollection* collection)
{
  vtkInternal::VectorOfViews::iterator iter;
  for (iter = this->Internal->Views.begin(); 
       iter != this->Internal->Views.end(); ++iter)
    {
    collection->AddItem(iter->GetPointer());
    }
}

//----------------------------------------------------------------------------
void vtkSMComparativeViewProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Dimensions: " << this->Dimensions[0] 
    << ", " << this->Dimensions[1] << endl;
}

