/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkXMLTextPropertyWriter.h,v $
  Language:  C++
  Date:      $Date: 2003-03-21 22:37:30 $
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
// .NAME vtkXMLTextPropertyWriter - vtkTextProperty XML Writer.
// .SECTION Description
// vtkXMLTextPropertyWriter provides XML writing functionality to 
// vtkTextProperty.

#ifndef __vtkXMLTextPropertyWriter_h
#define __vtkXMLTextPropertyWriter_h

#include "vtkObject.h"

class vtkTextProperty;

class VTK_EXPORT vtkXMLTextPropertyWriter : public vtkObject
{
public:
  static vtkXMLTextPropertyWriter* New();
  vtkTypeRevisionMacro(vtkXMLTextPropertyWriter,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get/Set the TextProperty to serialize.
  virtual void SetTextProperty(vtkTextProperty*);
  vtkGetObjectMacro(TextProperty, vtkTextProperty);

  // Description:
  // Write a serialized representation of the TextProperty
  virtual int Write(ostream &os, vtkIndent indent);

protected:
  vtkXMLTextPropertyWriter();
  ~vtkXMLTextPropertyWriter();  
  
  vtkTextProperty *TextProperty;

private:
  vtkXMLTextPropertyWriter(const vtkXMLTextPropertyWriter&);  // Not implemented.
  void operator=(const vtkXMLTextPropertyWriter&);  // Not implemented.
};

#endif
