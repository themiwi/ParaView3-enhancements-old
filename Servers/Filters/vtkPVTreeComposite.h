/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPVTreeComposite.h,v $
  Language:  C++
  Date:      $Date: 2001-05-02 19:23:01 $
  Version:   $Revision: 1.3 $
  
Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkPVTreeComposite - An interrupatble version of the superclass
// .SECTION Description
// vtkPVTreeComposite  is a sublcass of tree compsite that has methods that 
// interrupt rendering.  This functionality requires an MPI controller.
// .SECTION see also
// vtkMultiProcessController vtkRenderWindow.

#ifndef __vtkPVTreeComposite_h
#define __vtkPVTreeComposite_h

#include "vtkTreeComposite.h"
#include "vtkPVRenderView.h"

#ifndef _WIN32
//#ifndef VTK_USE_MPI

/*

class VTK_EXPORT vtkPVTreeComposite : public vtkTreeComposite
{
public:
  vtkPVTreeComposite() {};
  static vtkPVTreeComposite *New() { return new vtkPVTreeComposite;}
  vtkTypeMacro(vtkPVTreeComposite,vtkTreeComposite);

  void SetRenderView(vtkPVRenderView *v) {v = v;}  
  
protected:
  vtkPVTreeComposite(const vtkPVTreeComposite&) {};
  void operator=(const vtkPVTreeComposite&) {};
};

*/

#else



#include "vtkMPIController.h"

class VTK_EXPORT vtkPVTreeComposite : public vtkTreeComposite
{
public:
  static vtkPVTreeComposite *New();
  vtkTypeMacro(vtkPVTreeComposite,vtkTreeComposite);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Used by call backs.  Not intended to be called by the user.
  // Empty methods that can be used by the subclass to interupt a parallel render.
  virtual void CheckForAbortRender();
  virtual int CheckForAbortComposite();
  
  // Description:
  // The RenderView has methods for checking for events.
  vtkSetObjectMacro(RenderView, vtkPVRenderView);
  vtkGetObjectMacro(RenderView, vtkPVRenderView);
  
protected:
  vtkPVTreeComposite();
  ~vtkPVTreeComposite();
  vtkPVTreeComposite(const vtkPVTreeComposite&) {};
  void operator=(const vtkPVTreeComposite&) {};

  int LocalProcessId;
  int RenderAborted;
  vtkPVRenderView *RenderView;

  int SatelliteFinalAbortCheck();
  int SatelliteAbortCheck();
  int RootAbortCheck();
  int RootFinalAbortCheck();
  
  vtkMPIController *MPIController;

//BTX  
  // For the asynchronous receives.
  vtkMPICommunicator::Request ReceiveRequest;
  int ReceivePending;
  int ReceiveMessage;
//ETX  
  
  // Only used on the satellite processes.  When on, the root is 
  // waiting in a blocking receive.  It expects to be pinged so
  // it can check for abort requests.
  int RootWaiting;

  
};

// ifndef VTK_USE_MPI
#endif


// ifndef __vtkPVTreeComposite_h
#endif
