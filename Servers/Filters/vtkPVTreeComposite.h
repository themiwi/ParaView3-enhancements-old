/*=========================================================================
  
  Program:   ParaView
  Module:    $RCSfile: vtkPVTreeComposite.h,v $
  Language:  C++
  Date:      $Date: 2002-08-07 21:16:27 $
  Version:   $Revision: 1.18 $  
  
Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

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

#include "vtkToolkits.h" // Needed for VTK_USE_MPI
#ifndef __vtkPVTreeComposite_h
#define __vtkPVTreeComposite_h

#ifdef VTK_USE_MPI
#include "vtkMPICommunicator.h" // Needed for vtkMPICommunicator::Request
class vtkMPIController;
#endif

#include "vtkCompositeManager.h"

class vtkPVRenderView;

class VTK_EXPORT vtkPVTreeComposite : public vtkCompositeManager
{
public:
  static vtkPVTreeComposite *New();
  vtkTypeRevisionMacro(vtkPVTreeComposite,vtkCompositeManager);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Used by call backs.  Not intended to be called by the user.
  // Empty methods that can be used by the subclass to interupt a parallel render.
  virtual void CheckForAbortRender();
  virtual int CheckForAbortComposite();
  
  // Description:
  // The RenderView has methods for checking for events.
  virtual void SetRenderView(vtkPVRenderView*);
  vtkGetObjectMacro(RenderView, vtkPVRenderView);
  
  // Description:
  // This flag is on by default.
  // If this flag is off, then the behavior of this class becomes
  // that of the superclass (does not check for abort flag).
  vtkSetMacro(EnableAbort, int);
  vtkGetMacro(EnableAbort, int);
  vtkBooleanMacro(EnableAbort, int);

  // Description:
  // Avoid having the cross hairs contribute to the bounds unless
  // it is the only actor.  This method considers Pickable flag.
  virtual void ComputeVisiblePropBounds(vtkRenderer *ren, 
                                        float bounds[6]);


  // Description:
  // Also initialize rmi to check for data.
  virtual void InitializeRMIs();

  // Description:
  // Public because it is an RMI,  It checks to see if data exists
  // on the remote processes before bothering to composite.
  // It uses the supperclasses "UseComposite" flag to disable compositing.
  virtual void StartRender();

  // Description:
  // Public because it is an RMI.  
  void CheckForDataRMI();

protected:
  vtkPVTreeComposite();
  ~vtkPVTreeComposite();

  int  CheckForData();
  int  ShouldIComposite();

//BTX

  enum Tags {
    CHECK_FOR_DATA_TAG=442445
  };
//ETX

  int EnableAbort;

  int LocalProcessId;
  int RenderAborted;
  vtkPVRenderView *RenderView;
  int Printing;
  // Flag used to indicate the first call for a render.
  // There is no initialize method.
  int Initialized;

//BTX 
#ifdef VTK_USE_MPI 
  void SatelliteFinalAbortCheck();
  void SatelliteAbortCheck();
  void RootAbortCheck();
  void RootFinalAbortCheck();
  void RootWaitForSatelliteToFinish(int satelliteId);
  void RootSendFinalCompositeDecision();
  
  // For the asynchronous receives.
  vtkMPIController *MPIController;
  vtkMPICommunicator::Request ReceiveRequest;
  // Only used on the satellite processes.  When on, the root is 
  // waiting in a blocking receive.  It expects to be pinged so
  // it can check for abort requests.
  int RootWaiting;
  int ReceivePending;
  int ReceiveMessage;
#endif
//ETX  
    
  vtkPVTreeComposite(const vtkPVTreeComposite&); // Not implemented
  void operator=(const vtkPVTreeComposite&); // Not implemented
};

// ifndef __vtkPVTreeComposite_h
#endif
