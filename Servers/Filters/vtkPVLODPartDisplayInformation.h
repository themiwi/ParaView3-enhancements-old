/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVLODPartDisplayInformation.h,v $
  Language:  C++
  Date:      $Date: 2003-06-04 17:08:20 $
  Version:   $Revision: 1.2 $

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
// .NAME vtkPVLODPartDisplayInformation - Holds memory size of geometry.
// .SECTION Description
// This information object collects the memory size of the high res geometry
// and the LOD geometry.  They are used to determine when to use the
// LOD and when to collect geometry for local rendering.

#ifndef __vtkPVLODPartDisplayInformation_h
#define __vtkPVLODPartDisplayInformation_h


#include "vtkPVInformation.h"


class VTK_EXPORT vtkPVLODPartDisplayInformation : public vtkPVInformation
{
public:
  static vtkPVLODPartDisplayInformation* New();
  vtkTypeRevisionMacro(vtkPVLODPartDisplayInformation, vtkPVInformation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Memory sizes of full resolution geometry and decimated geometry 
  // summed over all processes.
  vtkGetMacro(GeometryMemorySize, int);
  vtkGetMacro(LODGeometryMemorySize, int);

  // Description:
  // Transfer information about a single object into
  // this object.
  virtual void CopyFromObject(vtkObject* data);
  virtual void CopyFromMessage(unsigned char* msg);

  // Description:
  // Merge another information object.
  virtual void AddInformation(vtkPVInformation* info);
  
  // Description:
  // Serialize message.
  virtual int GetMessageLength(); 
  virtual void WriteMessage(unsigned char* msg);

protected:
  vtkPVLODPartDisplayInformation() {};
  ~vtkPVLODPartDisplayInformation() {};

  // These used to be unsigned long, but I changed them to int
  // incase client is 32bit and server is 64bit.
  int GeometryMemorySize;
  int LODGeometryMemorySize;

  vtkPVLODPartDisplayInformation(const vtkPVLODPartDisplayInformation&); // Not implemented
  void operator=(const vtkPVLODPartDisplayInformation&); // Not implemented
};

#endif
