/*=========================================================================
  
  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPVTreeComposite.cxx,v $
  Language:  C++
  Date:      $Date: 2001-06-25 14:16:38 $
  Version:   $Revision: 1.12 $
  
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
CAUSED AND ON ANY THxEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVTreeComposite.h"
#include "vtkObjectFactory.h"


//-------------------------------------------------------------------------
vtkPVTreeComposite* vtkPVTreeComposite::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVTreeComposite");
  if(ret)
    {
    return (vtkPVTreeComposite*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVTreeComposite;
}


//#########################################################################
// If we are not using MPI, just stub out this class so the supper class
// will do every thing.
#ifdef VTK_USE_MPI

//-------------------------------------------------------------------------
vtkPVTreeComposite::vtkPVTreeComposite()
{
  this->MPIController = vtkMPIController::SafeDownCast(this->Controller);
  
  this->RootWaiting = 0;
  this->ReceivePending = 0;
  this->ReceiveMessage = 0;
  
  if (this->MPIController == NULL)
    {
    vtkErrorMacro("This objects requires an MPI controller.");
    }
  this->LocalProcessId = this->Controller->GetLocalProcessId();
  this->RenderAborted = 0;
  
  this->RenderView = NULL;
  this->Printing = 0;
  this->Initialized = 0;
}

  
//-------------------------------------------------------------------------
vtkPVTreeComposite::~vtkPVTreeComposite()
{
  this->MPIController = NULL;
  
  this->SetRenderView(NULL);
  
  // sanity check
  if (this->ReceivePending)
    {
    vtkErrorMacro("A receive is still pending.");
    }  
    
}


//----------------------------------------------------------------------------
void vtkPVTreeComposite::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkTreeComposite::PrintSelf(os, indent);
}


//----------------------------------------------------------------------------
void vtkPVTreeComposite::CheckForAbortRender()
{
  if (!this->RenderView)
    {
    return;
    }
  
  if ( ! this->Initialized)
    {
    // Never abort while printing.
    if (this->RenderView != NULL && this->RenderView->GetPrinting())
      {
      this->Printing = 1;
      }
    else
      {
      this->Printing = 0;
      }
    }
  
  this->Initialized = 1;
  
  
  if (this->LocalProcessId == 0)
    {
    this->RootAbortCheck();
    }
  else
    {
    this->SatelliteAbortCheck();
    }
}

//----------------------------------------------------------------------------
int vtkPVTreeComposite::CheckForAbortComposite()
{
  int abort;
  
  // Check for abort render has to be called at least once.
  if ( ! this->Initialized)
    {
    this->CheckForAbortRender();
    }
  
  if (this->LocalProcessId == 0)
    {
    this->RootFinalAbortCheck();
    }
  else
    {
    this->SatelliteFinalAbortCheck();
    }
  
  // Reset this for the next render.
  abort = this->RenderAborted;
  this->RenderAborted = 0;
  this->Initialized = 0;
  
  // Does this method really need to return a value?
  return abort;
}




#define VTK_STATUS_TAG           548934


// Root ------>>>>>>>> Satellite
// Exaclty one of these two methods will be sent (by the root) 
// to each satelite process.  They terminate the non blocking receive.  
// The second message is also used as a barrier to ensure all
// processes start compositing at the same time (Satellites wait for the request).
// Although this barrier is less efficient, I did not want the mess of 
// cancelling compositing.
#define  VTK_ABORT_RENDER              0
#define  VTK_COMPOSITE                 1


// Root ------>>>>>>>> Satellite
// When the root process has finished rendering, it waits for each of the satellite
// processes (one by one) to finish rendering.  This root sends this message
// to inform a satellite process that is waiting for it.  In a normal render 
// (not aborted) each satellite process will get exactly one of these messages.  
// If rendering has been aborted, the the root does not bother sending
// this message to the remaining satellites.
#define  VTK_ROOT_WAITING              2

// Root <<<<<<<<------ Satellite
// This message may be sent from any satellite processes to the root processes.
// It is used to ping the root process when it has finished rendering and is waiting 
// in a blocking receive for a "Finshed" message.  Only the processes that is currently 
// being waited on can send these messages.  Any number of them can be sent (including 0).
#define  VTK_CHECK_ABORT               3

// Root <<<<<<<<------ Satellite
// This message is sent from the satellite (the root is actively waiting for) 
// to signal that is done rendering and is waiting to composite.  
// In a normal (not aborted) render, every satellite process sends this 
// message exactly once to the root.
#define  VTK_FINISHED                  4



//------------- Methods for Root Processes --------------

// Two satellites, possible message traces.
// Root:
// No Abort,
//----------
// send 1 ROOT_WAITING
// [rec 1 CHECK_ABORT]
// ... repeat any number of times ...               
// rec  1 FINISHED    
// send 2 ROOT_WAITING
// [rec 2 CHECK_ABORT]
// ... repeat any number of times ...               
// rec  2 FINISHED    
// send 1 COMPOSITE    
// send 2 COMPOSITE    

// Abort during waiting for 2.
//----------
// send 1 ROOT_WAITING
// [rec 1 CHECK_ABORT]
// ... repeat any number of times ...               
// rec  1 FINISHED    
// send 2 ROOT_WAITING
// [rec 2 CHECK_ABORT]
// ... repeat any number of times ...
// send 1 ABORT
// send 2 ABORT
// rec  2 FINISHED    

// Abort during waiting for 1.
//----------
// send 1 ROOT_WAITING
// [rec 1 CHECK_ABORT]
// ... repeat any number of times ...               
// send 1 ABORT
// send 2 ABORT
// rec  1 FINISHED    

// Abort during render.
//----------
// send 1 ABORT
// send 2 ABORT

//            Abort during waiting.    Abort during render


//-------------------------------------------------------------------------
// Count is temporary for testing.
void vtkPVTreeComposite::RootAbortCheck()
{
  //sleep(5);
  int abort;

  // If the render has already been aborted, then we need do nothing else.
  if (this->RenderAborted)
    {
    return;
    }

  // This checks for events to decide whether to abort.
  abort = this->RenderView->ShouldIAbort();
  if ( ! this->Printing && this->RenderView && abort)
    { // Yes, abort.
    int idx;
    int message;
    int num = this->MPIController->GetNumberOfProcesses();

    // Tell the satellite processes they need to abort.
    for (idx = 1; idx < num; ++idx)
      {
      //cout << "0 send to 1, message: " << message << endl;
      message = VTK_ABORT_RENDER;
      this->MPIController->Send(&message, 1, idx, VTK_STATUS_TAG);
      }
    
    // abort our own render.
    this->RenderWindow->SetAbortRender(1);
    this->RenderAborted = 1;
    // For some abort types, we want to queue another render.
    if (abort == 1)
      {
      this->RenderView->EventuallyRender();
      }
    }
}


//-------------------------------------------------------------------------
// "abort" is true if rendering was previously aborted.
void vtkPVTreeComposite::RootFinalAbortCheck()
{
  int idx;
  int num;

  // If the render has already been aborted, then we need do nothing else.
  if (this->RenderAborted)
    {
    return;
    }
  
  // Wait for all the satelite processes to finish.
  num = this->MPIController->GetNumberOfProcesses();
  for (idx = 1; idx < num; ++idx)
    {
    // An abort could have occured while waiting for a satellite.
    if ( ! this->RenderAborted)
      {
      this->RootWaitForSatelliteToFinish(idx);
      }
    }

  // Sends the final message to all satellites.
  this->RootSendFinalCompositeDecision();
}



//-------------------------------------------------------------------------
// This only gets called when there has not been an abort. 
// If an abort occured during the call, then this method returns 1.
// It returns 0 if no abort occured.
void vtkPVTreeComposite::RootWaitForSatelliteToFinish(int satelliteId)
{
  int message;

  // Send a message to the process that informs it 
  // that we are waiting for it to finish rendering,
  // and expect to be pinged every so often.
  message = VTK_ROOT_WAITING;
  this->MPIController->Send(&message, 1, satelliteId, VTK_STATUS_TAG);
  
  // Wait for the process to finish.
  while (1)
    {
    this->MPIController->Receive(&message, 1, satelliteId, VTK_STATUS_TAG);

    // Even if we abort, We still expect the "FINISHED" message because
    // the satellite might sned the "FINISHED" message before it receives
    // the "ABORT" message.
    if (message == VTK_FINISHED)
      {
      return;
      }
    else if (message == VTK_CHECK_ABORT)
      {
      // The satellite is in the middle of a long render and has pinged us to
      // check for an abort.  This call sends the abort messages internally.
      this->RootAbortCheck();
      }
    else 
      {
      vtkErrorMacro("Sanity check failed: Expecting CheckAbort or Finished message.");
      }
    }
}



//-------------------------------------------------------------------------
// This method has simplified to the point that it could be eliminated.
void vtkPVTreeComposite::RootSendFinalCompositeDecision()
{
  int message;
  int idx, num;
  
  // If ABORT was already sent, then we do not need to worry about the
  // composite.  It is already cancelled.
  num = this->MPIController->GetNumberOfProcesses();
  if ( ! this->RenderAborted)
    {
    for (idx = 1; idx < num; ++idx)
      {
      // In order to get rid of the abort asych receive still pending.
      message = VTK_COMPOSITE;
      this->MPIController->Send(&message, 1, idx, VTK_STATUS_TAG);
      }
    }
}



//------------- Methods for Satellite Processes --------------


//-------------------------------------------------------------------------
void vtkPVTreeComposite::SatelliteAbortCheck()
{
  int message;

  if (this->RenderAborted)
    {
    return;
    }
  
  // If the root is waiting on us, then ping it so that it can check for an abort.
  if (this->RootWaiting)
    {
    //cout << "1: Ping root\n";
    vtkMPICommunicator::Request sendRequest;
    message = VTK_CHECK_ABORT;
    //cout << "1 noBlockSend to 0, message: " << status << endl;
    this->MPIController->NoBlockSend(&message, 1, 0, VTK_STATUS_TAG, sendRequest);
    }
  
  // If this is the first call for this render, 
  // then we need to setup the receive message.
  if ( ! this->ReceivePending)
    {
    this->MPIController->NoBlockReceive(&this->ReceiveMessage, 1, 0, VTK_STATUS_TAG, 
					this->ReceiveRequest);
    this->ReceivePending = 1;
    }
  
  if (this->ReceivePending && this->ReceiveRequest.Test())
    { // Received a message from the root.
    this->ReceivePending = 0;
    
    // It could be ABORT, or ROOT_WAITING.  It could not be COMPOSITE
    // because that can only be called after root receives our FINISHED message.
    
    if (this->ReceiveMessage == VTK_ABORT_RENDER)
      {  // Root is telling us to short circuit the render.
      // .... set abort flag of render window.....
      this->RenderWindow->SetAbortRender(1);
      this->RenderAborted = 1;
      // Do NOT rearm to asynchronous receive.
      return;
      }
    else if (this->ReceiveMessage == VTK_ROOT_WAITING)
      { // Root is finished rendering, and is waiting for this process.  
      // It wants to be pinged occasionally so it can check for aborts.
      this->RootWaiting = 1;
      // Rearm the receive to get a possible abort.
      this->MPIController->NoBlockReceive(&this->ReceiveMessage, 1, 0, 
					  VTK_STATUS_TAG, this->ReceiveRequest);
      this->ReceivePending = 1;
      }
    else
      {
      vtkErrorMacro("Expecting ABORT or WAITING from root.");
      }
    }
  
  return;
}





//-------------------------------------------------------------------------
// Out process is finished rendering now and will wait for the final status 
// message from root.
void vtkPVTreeComposite::SatelliteFinalAbortCheck()
{
  int message;
  
  // We can not send a FINISHED message until the root is WAITING.
  if ( ! this->RootWaiting && ! this->RenderAborted)
    {
    // Wait for one of these messages: ROOT_WAITING, or ABORT
    if (this->ReceivePending)
      {
      this->ReceiveRequest.Wait();
      this->ReceivePending = 0;
      if (this->ReceiveMessage == VTK_ABORT_RENDER)
	{  // Root is telling us to short circuit the render.
	// We we have received a ROOT_WAITING message, then we have to send
	// a FINISHED message (event if an ABORT has been received meanwhile).
	this->RenderAborted = 1;
	}      
      else if (this->ReceiveMessage == VTK_ROOT_WAITING)
	{ 
	this->RootWaiting = 1;
	// Rearm the receive to put in a consistent state.
	this->MPIController->NoBlockReceive(&this->ReceiveMessage, 1, 0, 
					    VTK_STATUS_TAG, this->ReceiveRequest);
	this->ReceivePending = 1;
	}
      else 
	{
	vtkErrorMacro("Expecting ROOT_WAITING or ABORT message from root.");
	}
      }
    }
  
  
  // We we have received a ROOT_WAITING message, then we have to send
  // a FINISHED message (event if an ABORT has been received meanwhile).
  if ( this->RootWaiting)
    {
    message = VTK_FINISHED;
    this->MPIController->Send(&message, 1, 0, VTK_STATUS_TAG);
    // Reset the RootWaiting flag for the next render.
    this->RootWaiting = 0;
    }
  
  // If there has already been an ABORT, then receive will no longer be pending.
  
  // Now tie up any loose ends.
  // Wait for one of these messages: COMPOSITE, or ABORT
  // We are gaurenteed to get exactly on of these two.
  // The receive would not still be pending if we received one already.
  if (this->ReceivePending)
    {
    this->ReceiveRequest.Wait();
    this->ReceivePending = 0;
    if (this->ReceiveMessage == VTK_ABORT_RENDER)
      {  // Root is telling us to short circuit the render.
      this->RenderAborted = 1;
      }
    else if (this->ReceiveMessage == VTK_COMPOSITE)
      {
      // We do not need to do anything here.
      }
    }
}



// end VTK_USE_MPI
//#########################################################################
#else
// Not using MPI

//-------------------------------------------------------------------------
vtkPVTreeComposite::vtkPVTreeComposite()
{ 
  this->RenderView = NULL;
}

  
//-------------------------------------------------------------------------
vtkPVTreeComposite::~vtkPVTreeComposite()
{
  this->SetRenderView(NULL);
}


//----------------------------------------------------------------------------
void vtkPVTreeComposite::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkTreeComposite::PrintSelf(os, indent);
}


//----------------------------------------------------------------------------
void vtkPVTreeComposite::CheckForAbortRender()
{
  this->vtkTreeComposite::CheckForAbortRender();
}


//----------------------------------------------------------------------------
int vtkPVTreeComposite::CheckForAbortComposite()
{
  return this->vtkTreeComposite::CheckForAbortComposite();
}



#endif  // VTK_USE_MPI





