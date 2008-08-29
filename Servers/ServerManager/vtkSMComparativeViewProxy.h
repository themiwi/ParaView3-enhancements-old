/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMComparativeViewProxy.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMComparativeViewProxy - view for comparative visualization/
// film-strips.
// .SECTION Description
// vtkSMComparativeViewProxy is the view used to generate/view comparative
// visualizations/film-strips.

#ifndef __vtkSMComparativeViewProxy_h
#define __vtkSMComparativeViewProxy_h

#include "vtkSMViewProxy.h"

class vtkSMAnimationSceneProxy;
class vtkCollection;

class VTK_EXPORT vtkSMComparativeViewProxy : public vtkSMViewProxy
{
public:
  static vtkSMComparativeViewProxy* New();
  vtkTypeRevisionMacro(vtkSMComparativeViewProxy, vtkSMViewProxy);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  enum Types
    {
    FILM_STRIP,
    COMPARATIVE
    };
  //ETX

  //BTX
  enum ViewUpdateModeTypes
    {
    UPDATE_MODE_NONE,
    UPDATE_MODE_ROOT,
    UPDATE_MODE_ALL
    };
  //ETX
  
  // Description:
  // Set the mode. In FILM_STRIP mode, AnimationSceneX is used while in
  // COMPARATIVE mode AnimationSceneX as well as AnimationSceneY are used.
  vtkSetMacro(Mode, int);
  vtkGetMacro(Mode, int);

  // Description:
  // This property determines how individual frames of the comparative view will
  // update when a source or filter displayed by the view is modified.  When UPDATE_MODE_NONE is
  // selected, no frames will update until the comparative view is explicity updated. 
  // When UPDATE_MODE_ROOT is selected, the root frame will update but the other frames will not
  // update until the comparative view is explicity updated.  When UPDATE_MODE_ALL is selected, all
  // frames will update.
  vtkSetMacro(ViewUpdateMode, int);
  vtkGetMacro(ViewUpdateMode, int);

  // Description:
  // This property controls how the comparative view displays time.  When show timesteps
  // is off, the global time is displayed in each frame.  When show timesteps is on, the
  // comparative view displays different timesteps in each frame.  When show timesteps is
  // on, the global time is ignored.
  vtkSetMacro(ShowTimeSteps, int);
  vtkGetMacro(ShowTimeSteps, int);

  // Description:
  // Builds the MxN views. This method simply creates the MxN internal view modules.
  // It does not generate the visualization i.e. play the animation scene(s).
  // This method does nothing unless the dimensions have changed, in which case
  // it creates new internal view modules (or destroys extra ones). Note that
  // the it's the responsibility of the application to lay the views out so that
  // they form a MxN grid. 
  void Build(int dx, int dy);

  // Description:
  // Returns the dimensions used by the most recent Build() request.
  vtkGetVector2Macro(Dimensions, int);

  // Description:
  // Adds a representation proxy to this view. 
  virtual void AddRepresentation(vtkSMRepresentationProxy*);

  // Description:
  // Removes a representation proxy from this view.
  virtual void RemoveRepresentation(vtkSMRepresentationProxy*);

  // Description:
  // Removes all added representations from this view.
  // Simply calls RemoveRepresentation() on all added representations 
  // one by one.
  virtual void RemoveAllRepresentations();

  // Description:
  // Forwards the call to all internal views.
  virtual void StillRender();

  // Description:
  // Renders the view using lower resolution is possible.
  // Forwards the call to all internal views.
  virtual void InteractiveRender();

  // Description:
  // Create a default representation for the given source proxy.
  // Returns a new proxy.
  // Overridden to forward the call to the internal root view proxy.
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(vtkSMProxy*, int);
  virtual vtkSMRepresentationProxy* CreateDefaultRepresentation(vtkSMProxy* proxy)
    { return this->Superclass::CreateDefaultRepresentation(proxy); }

  // Description:
  // Get all the internal views. The views should only be used to be layed out
  // by the GUI. It's not recommended to directly change the properties of the
  // views.
  void GetViews(vtkCollection* collection);

  // Description:
  // Get all internal vtkSMRepresentations for a given view.  If the given
  // view is not managed by this comparative view it will be ignored.  The
  // representations should only be used by the GUI for creating representation
  // clones.  It is not recommended to directly change the properties of the returned
  // representations.
  void GetRepresentationsForView(vtkSMViewProxy*, vtkCollection*);

  // Description:
  // Returns the root view proxy.
  vtkSMViewProxy* GetRootView();

  // Description:
  // Called on every still render. This checks if the comparative visualization
  // needs to be regenerated (following changes to proxies involved in
  // generating the comparative visualization)/tim
  void UpdateVisualization(int force=0);

  // Description:
  // ViewSize, ViewPosition need to split up among all the component
  // views correctly.
  virtual void SetViewSize(int x, int y)
    {
    this->ViewSize[0] = x;
    this->ViewSize[1] = y;
    this->UpdateViewLayout();
    }

  // Description:
  // ViewSize, ViewPosition need to split up among all the component
  // views correctly.
  virtual void SetViewPosition(int x, int y)
    {
    this->Superclass::SetViewPosition(x, y);
    this->UpdateViewLayout();
    }

  // Description:
  // ViewSize, ViewPosition need to split up among all the component
  // views correctly.
  virtual void SetGUISize(int x, int y)
    {
    this->Superclass::SetGUISize(x, y);
    this->UpdateViewLayout();
    }

  // Description:
  // Set spacing between views. 
  vtkSetVector2Macro(Spacing, int);
  vtkGetVector2Macro(Spacing, int);

  // Description:
  // Set the time range and mark the scene outdated.
  void SetTimeRange (double min, double max)
    {
    if ( (this->TimeRange[0] != min) || (this->TimeRange[1] != max))
      {
      this->TimeRange[0] = min;
      this->TimeRange[1] = max;
      this->Modified();
      this->MarkSceneOutdated();
      }
    }

  void SetTimeRange(double x[2])
    {
    this->SetTimeRange(x[0], x[1]);
    }
  vtkGetVector2Macro(TimeRange, double);

  // Description:
  // Gets the X property and element number currently being compared, if any.
  virtual bool GetXPropertyAndElement(vtkSMProperty *&, int &);

  // Description:
  // Gets the Y property and element currently being compared, if any.
  virtual bool GetYPropertyAndElement(vtkSMProperty *&, int &);

  // Description:
  // Generally each view type is different class of view eg. bar char view, line
  // plot view etc. However in some cases a different view types are indeed the
  // same class of view the only different being that each one of them works in
  // a different configuration eg. "RenderView" in builin mode, 
  // "IceTDesktopRenderView" in remote render mode etc. This method is used to
  // determine what type of view needs to be created for the given class. When
  // user requests the creation of a view class, the application can call this
  // method on a prototype instantaiated for the requested class and the
  // determine the actual xmlname for the view to create.
  // Overridden to choose the correct type of render view.
 virtual const char* GetSuggestedViewType(vtkIdType connectionID);

//BTX
protected:
  vtkSMComparativeViewProxy();
  ~vtkSMComparativeViewProxy();

  // Description:
  // Called at start of CreateVTKObjects().
  virtual bool BeginCreateVTKObjects();

  // Description:
  // Creates and appends a new internal view.
  // This not only creates a new view but also new copies of representations
  // for all the representations in the view and adds them to the new view.
  void AddNewView();

  // Description:
  // Removes an internal view and all the representations in that view.
  void RemoveView(vtkSMViewProxy* remove);

  void UpdateRootView();

  // Description:
  // Update comparative scene.
  void UpdateComparativeVisualization(vtkSMAnimationSceneProxy* sceneX,
    vtkSMAnimationSceneProxy* sceneY);

  // Description:
  // Update timestrip scene.
  void UpdateFilmStripVisualization(vtkSMAnimationSceneProxy* scene);

  // Description:
  // Update layout for internal views.
  void UpdateViewLayout();

  // Description:
  // Marks the view dirty i.e. on next StillRender it needs to regenerate the
  // comparative vis by replaying the animation(s).
  void MarkSceneOutdated()
    { this->SceneOutdated=true; }

  int Mode;
  int ViewUpdateMode;
  int ShowTimeSteps;
  int Dimensions[2];
  int ViewSize[2];
  int Spacing[2];
  double TimeRange[2];

  vtkSMAnimationSceneProxy* AnimationSceneX;
  vtkSMAnimationSceneProxy* AnimationSceneY;

  bool SceneOutdated;

private:
  vtkSMComparativeViewProxy(const vtkSMComparativeViewProxy&); // Not implemented
  void operator=(const vtkSMComparativeViewProxy&); // Not implemented

  class vtkInternal;
  vtkInternal* Internal;
  vtkCommand* SceneObserver;

//ETX
};

#endif

