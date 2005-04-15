/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkSMAnimationSceneProxy.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMAnimationSceneProxy - proxy for vtkAnimationScene
// .SECTION Description
// .SECTION See Also
// vtkSMProxy vtkSMAnimationCueProxy
//

#ifndef __vtkSMAnimationSceneProxy_h
#define __vtkSMAnimationSceneProxy_h

#include "vtkSMAnimationCueProxy.h"

class vtkAnimationScene;
class vtkCollection;
class vtkCollectionIterator;
class vtkSMRenderModuleProxy;

class VTK_EXPORT vtkSMAnimationSceneProxy : public vtkSMAnimationCueProxy
{
public:
  static vtkSMAnimationSceneProxy* New();
  vtkTypeRevisionMacro(vtkSMAnimationSceneProxy, vtkSMAnimationCueProxy);
  void PrintSelf(ostream& os, vtkIndent indent);


  virtual void SaveInBatchScript(ofstream*);

  void Play();
  void Stop();
  int IsInPlay();
  
  void SetLoop(int loop);
  int GetLoop();

  void SetFrameRate(double framerate);
  double GetFrameRate();

  void SetPlayMode(int mode);
  int GetPlayMode();

  void AddCue(vtkSMProxy* cue);
  void RemoveCue(vtkSMProxy* cue);
  

  // Description:
  // Set the RenderModule Proxy.
  // This proxy must be set only in batch mode.
  // If set, every Tick event leads to a call to StillRender on the 
  // RenderModule.
  void SetRenderModuleProxy(vtkSMRenderModuleProxy*);
  
  void SetCurrentTime(double time);
protected:
  vtkSMAnimationSceneProxy();
  ~vtkSMAnimationSceneProxy();

  virtual void CreateVTKObjects(int numObjects);

  // Description:
  // Callbacks for corresponding Cue events. The argument must be 
  // casted to vtkAnimationCue::AnimationCueInfo.
  virtual void StartCueInternal(void* info);
  virtual void TickInternal(void* info);
  virtual void EndCueInternal(void* info);
  
  vtkCollection* AnimationCueProxies;
  vtkCollectionIterator* AnimationCueProxiesIterator;

  vtkSMRenderModuleProxy* RenderModuleProxy;
private:
  vtkSMAnimationSceneProxy(const vtkSMAnimationSceneProxy&); // Not implemented.
  void operator=(const vtkSMAnimationSceneProxy&); // Not implemented.
};


#endif

