/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkXMLPImageDataWriter.cxx,v $
  Language:  C++
  Date:      $Date: 2002-10-16 18:23:06 $
  Version:   $Revision: 1.1 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPImageDataWriter.h"
#include "vtkObjectFactory.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkImageData.h"

vtkCxxRevisionMacro(vtkXMLPImageDataWriter, "$Revision: 1.1 $");
vtkStandardNewMacro(vtkXMLPImageDataWriter);

//----------------------------------------------------------------------------
vtkXMLPImageDataWriter::vtkXMLPImageDataWriter()
{
}

//----------------------------------------------------------------------------
vtkXMLPImageDataWriter::~vtkXMLPImageDataWriter()
{
}

//----------------------------------------------------------------------------
void vtkXMLPImageDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkXMLPImageDataWriter::SetInput(vtkImageData* input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkImageData* vtkXMLPImageDataWriter::GetInput()
{
  if(this->NumberOfInputs < 1)
    {
    return 0;
    }
  
  return static_cast<vtkImageData*>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
const char* vtkXMLPImageDataWriter::GetDataSetName()
{
  return "PImageData";
}

//----------------------------------------------------------------------------
const char* vtkXMLPImageDataWriter::GetDefaultFileExtension()
{
  return "pvti";
}

//----------------------------------------------------------------------------
void vtkXMLPImageDataWriter::WritePrimaryElementAttributes()
{
  this->Superclass::WritePrimaryElementAttributes();
  vtkImageData* input = this->GetInput();
  this->WriteVectorAttribute("Origin", 3, input->GetOrigin());
  this->WriteVectorAttribute("Spacing", 3, input->GetSpacing());
}

//----------------------------------------------------------------------------
vtkXMLStructuredDataWriter*
vtkXMLPImageDataWriter::CreateStructuredPieceWriter()
{  
  // Create the writer for the piece.
  vtkXMLImageDataWriter* pWriter = vtkXMLImageDataWriter::New();
  pWriter->SetInput(this->GetInput());
  return pWriter;
}
