/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkKWRemoteExecute.cxx,v $
  Language:  C++
  Date:      $Date: 2003-04-17 20:25:12 $
  Version:   $Revision: 1.5 $

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
#include "vtkKWRemoteExecute.h"

#include "vtkObjectFactory.h"
#include "vtkString.h"
#include "vtkMultiThreader.h"

#ifdef _MSC_VER
#pragma warning (push, 2)
#endif

#include <vector>
#include <map>
#include <string>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef _WIN32
#else
# include <sys/types.h>
# include <unistd.h>
#endif

//----------------------------------------------------------------------------
//============================================================================
class vtkKWRemoteExecuteInternal
{
public:
  vtkKWRemoteExecuteInternal()
    {
    }
  typedef vtkstd::vector<vtkstd::string> VectorOfStrings;
  VectorOfStrings Args;
};
//============================================================================
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWRemoteExecute );
vtkCxxRevisionMacro(vtkKWRemoteExecute, "$Revision: 1.5 $");

//----------------------------------------------------------------------------
vtkKWRemoteExecute::vtkKWRemoteExecute()
{
  this->Internals = new vtkKWRemoteExecuteInternal;
  this->RemoteHost = 0;
  this->ProcessRunning = 0;
  this->Result = NOT_RUN;

  this->SSHCommand = 0;
  this->SSHArguments = 0;

  this->SetSSHCommand("ssh");
}

//----------------------------------------------------------------------------
vtkKWRemoteExecute::~vtkKWRemoteExecute()
{
  delete this->Internals;
  this->SetRemoteHost(0);

  this->SetSSHCommand(0);
  this->SetSSHArguments(0);
}

//----------------------------------------------------------------------------
int vtkKWRemoteExecute::Detach()
{
  int res = VTK_ERROR;
  cout << "Detaching ParaView" << endl;
#ifdef _WIN32
  // No code for detaching yet.
  vtkGenericWarningMacro("Cannot detach on windows yet");
#elif __linux__
  if ( daemon(0,0) == -1 )
    {
    vtkGenericWarningMacro("Problem detaching ParaView");
    return VTK_ERROR;
    }
  res = VTK_OK;
#else
  vtkGenericWarningMacro("Cannot detach on this system yet");
#endif
  return res;
}

//----------------------------------------------------------------------------
int vtkKWRemoteExecute::RunRemoteCommand(const char*, 
  const char* args[])
{
  if ( !this->RemoteHost )
    {
    vtkErrorMacro("Remote host not set");
    return 0;
    }

  if ( !this->SSHCommand )
    {
    vtkErrorMacro("SSH Command not set");
    return 0;
    }
  int res = 0;
  int cc;
  for ( cc = 0; args[cc]; cc ++ )
    {
    this->Internals->Args.push_back(args[cc]);
    }
  cout << "This is: " << this << endl;
  vtkMultiThreader* th = vtkMultiThreader::New();
  int tid = th->SpawnThread(
    static_cast<vtkThreadFunctionType>(vtkKWRemoteExecute::RunCommandThread), this);
  this->ProcessRunning = 1;
  this->Result = vtkKWRemoteExecute::RUNNING;

  while( this->Result == vtkKWRemoteExecute::RUNNING )
    {
    sleep(2);
    cout << "Waiting" << endl;
    }
  if ( this->Result == vtkKWRemoteExecute::SUCCESS )
    {
    res = 1;
    }
  th->TerminateThread(tid);
  th->Delete();
  return res;
}

//----------------------------------------------------------------------------
int vtkKWRemoteExecute::RunCommand(const char* command, const char* args[])
{
#ifdef __WITH__FORK__
  int res = 0;
#ifdef _WIN32
#else
  pid_t pid;
  if ( (pid = fork()) < 0 )
    {
    vtkErrorMacro("Cannot fork");
    return 0;
    }
  if ( pid == 0 )
    {
    /*
    if ( daemon(0,0) == -1 )
      {
      vtkErrorMacro("Cannot start process");
      return 0;
      }
      */
    execv(command, (char* const*)args);
    return 0;
    }
  cout << "Child's pid: " << pid << endl;
  res = pid;
#endif
  return res;
#endif

  int cc;
  ostrstream str;
  str << command;
  for ( cc = 0; args[cc]; cc ++ )
    {
    str << " \"" << args[cc] << "\"";
    }
  str << ends;
  cout << "Execute [" << str.str() << "]" << endl;
  system(str.str());
  str.rdbuf()->freeze(0);
  return VTK_OK;
}

//----------------------------------------------------------------------------
void* vtkKWRemoteExecute::RunCommandThread(void* vargs)
{
  vtkMultiThreader::ThreadInfo *ti = static_cast<vtkMultiThreader::ThreadInfo*>(vargs);
  vtkKWRemoteExecute* rw = static_cast<vtkKWRemoteExecute*>(ti->UserData);
  if ( !rw )
    {
    cout << "Have no pointer to RW" << endl;
    rw->Result = vtkKWRemoteExecute::FAIL;
    return 0;
    }

  cout << "Rw is " << rw << endl;

  vtkKWRemoteExecuteInternal::VectorOfStrings &args = 
    rw->Internals->Args;

  int cc;
  int cnt=args.size()+5;
  cout << "Number of arguments: " << cnt << endl;
  char** rargs = new char*[ cnt + 3];
  int scnt=0;
  if ( rw->SSHArguments )
    {
    rargs[scnt] = vtkString::Duplicate( rw->SSHArguments);
    scnt ++;
    }

  rargs[scnt] = vtkString::Duplicate( rw->RemoteHost);
  scnt ++;

  cout << "Prepend: " << rargs[0] << endl;

  for ( cc = 0; cc < args.size(); cc ++ )
    {
    rargs[scnt] = vtkString::Duplicate( args[cc].c_str() );
    scnt ++;
    }
  rargs[scnt] = 0;
  int res = rw->RunCommand(rw->SSHCommand, (const char**)rargs);
  for ( cc = 0; rargs[cc]; cc ++ )
    {
    delete [] rargs[cc];
    }
  delete [] rargs;
  if ( res == VTK_OK )
    {
    rw->Result = vtkKWRemoteExecute::SUCCESS;
    }
  else
    {
    rw->Result = vtkKWRemoteExecute::FAIL;
    }
  if ( rw ) 
    {
    rw->ProcessRunning = 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWRemoteExecute::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "RemoteHost: " << this->RemoteHost << endl;
}
