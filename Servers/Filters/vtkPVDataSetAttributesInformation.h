/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVDataSetAttributesInformation.h,v $
  Language:  C++
  Date:      $Date: 2002-12-04 19:05:23 $
  Version:   $Revision: 1.1 $

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
// .NAME vtkPVDataSetAttributesInformation - List of array info
// .SECTION Description
// Information associated with vtkDataSetAttributes object (i.e point data).
// This object does not have any user interface.  It is created and destroyed
// on the fly as needed.  It may be possible to add features of this object
// to vtkDataSetAttributes.  That would eliminate all of the "Information"
// in ParaView.

#ifndef __vtkPVDataSetAttributesInformation_h
#define __vtkPVDataSetAttributesInformation_h


#include "vtkObject.h"

class vtkDataSetAttributes;
class vtkPVArrayInformation;
class vtkCollection;

class VTK_EXPORT vtkPVDataSetAttributesInformation : public vtkObject
{
public:
  static vtkPVDataSetAttributesInformation* New();
  vtkTypeRevisionMacro(vtkPVDataSetAttributesInformation, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Transfer information about a single vtk data object into
  // this object.
  void CopyFromDataSetAttributes(vtkDataSetAttributes* data);
  void DeepCopy(vtkPVDataSetAttributesInformation* info);

  // Description:
  // Intersect information of argument with information currently
  // in this object.  Arrays must be in both 
  // (same name and number of components)to be in final.        
  void AddInformation(vtkDataSetAttributes* da);
  void AddInformation(vtkPVDataSetAttributesInformation* info);
  
  // Description:
  // Remove all infommation. next add will be like a copy.
  void Initialize();

  // Description:
  // Access to information.
  int                    GetNumberOfArrays();
  vtkPVArrayInformation* GetArrayInformation(int idx);
  vtkPVArrayInformation* GetArrayInformation(const char *name);

  // Description:
  // For getting default scalars ... (vtkDataSetAttributes::SCALARS).
  vtkPVArrayInformation* GetAttributeInformation(int attributeType);
  
  // Description:
  // Mimicks data set attribute call.  Returns -1 if array (of index) is
  // not a standard attribute.  Returns attribute type otherwise.
  int IsArrayAnAttribute(int arrayIndex);

  // Description:
  // Stuff for sending this object to other processes.
  // I will worry about byte swapping later.
  // CopyFromMessage returns how many bytes were used from the
  // message (same as message length after call).
  int GetMessageLength();
  int WriteMessage(unsigned char *msg);
  int CopyFromMessage(unsigned char *msg);

protected:
  vtkPVDataSetAttributesInformation();
  ~vtkPVDataSetAttributesInformation();

  // Data information collected from remote processes.
  vtkCollection* ArrayInformation;
  // Standard cell attributes.
  int            AttributeIndices[5]; 

  vtkPVDataSetAttributesInformation(const vtkPVDataSetAttributesInformation&); // Not implemented
  void operator=(const vtkPVDataSetAttributesInformation&); // Not implemented
};

#endif
