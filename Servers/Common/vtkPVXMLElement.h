/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile: vtkPVXMLElement.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVXMLElement represents an XML element and those nested inside.
// .SECTION Description
// This is used by vtkPVXMLParser to represent an XML document starting
// at the root element.
#ifndef __vtkPVXMLElement_h
#define __vtkPVXMLElement_h

#include "vtkObject.h"

class vtkPVXMLParser;

//BTX
struct vtkPVXMLElementInternals;
//ETX

class VTK_EXPORT vtkPVXMLElement : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkPVXMLElement,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPVXMLElement* New();

  // Description:
  // Get the name of the element.  This is its XML tag.
  vtkGetStringMacro(Name);

  // Description:
  // Get the id of the element.
  vtkGetStringMacro(Id);

  // Description:
  // Get the attribute with the given name.  If it doesn't exist,
  // returns 0.
  const char* GetAttribute(const char* name);

  // Description:
  // Get the attribute with the given name and converted to a scalar
  // value.  Returns whether value was extracted.
  int GetScalarAttribute(const char* name, int* value);
  int GetScalarAttribute(const char* name, float* value);
  int GetScalarAttribute(const char* name, double* value);

  // Description:
  // Get the attribute with the given name and converted to a scalar
  // value.  Returns length of vector read.
  int GetVectorAttribute(const char* name, int length, int* value);
  int GetVectorAttribute(const char* name, int length, float* value);
  int GetVectorAttribute(const char* name, int length, double* value);

  // Description:
  // Get the parent of this element.
  vtkPVXMLElement* GetParent();

  // Description:
  // Get the number of elements nested in this one.
  unsigned int GetNumberOfNestedElements();

  // Description:
  // Get the element nested in this one at the given index.
  vtkPVXMLElement* GetNestedElement(unsigned int index);

  // Description:
  // Find a nested element with the given id.
  vtkPVXMLElement* FindNestedElement(const char* id);

  // Description:
  // Lookup the element with the given id, starting at this scope.
  vtkPVXMLElement* LookupElement(const char* id);

  // Description:
  // Given it's name and value, add an attribute.
  void AddAttribute(const char* attrName, const char* attrValue);
  void AddAttribute(const char* attrName, unsigned int attrValue);
  void AddAttribute(const char* attrName, double attrValue);
  void AddAttribute(const char* attrName, int attrValue);
#if defined(VTK_USE_64BIT_IDS)
  void AddAttribute(const char* attrName, vtkIdType attrValue);
#endif

  // Description:
  // Add a sub-element. The parent element keeps a reference to
  // sub-element.
  void AddNestedElement(vtkPVXMLElement* element);

  vtkSetStringMacro(Name);

  void PrintXML(ostream& os, vtkIndent indent);

protected:
  vtkPVXMLElement();
  ~vtkPVXMLElement();

  vtkPVXMLElementInternals* Internal;

  char* Name;
  char* Id;

  // The parent of this element.
  vtkPVXMLElement* Parent;

  // Method used by vtkPVXMLParser to setup the element.
  vtkSetStringMacro(Id);
  void ReadXMLAttributes(const char** atts);
  void AddCharacterData(const char* data, int length);


  // Internal utility methods.
  vtkPVXMLElement* LookupElementInScope(const char* id);
  vtkPVXMLElement* LookupElementUpScope(const char* id);
  void SetParent(vtkPVXMLElement* parent);

  //BTX
  friend class vtkPVXMLParser;
  //ETX

private:
  vtkPVXMLElement(const vtkPVXMLElement&);  // Not implemented.
  void operator=(const vtkPVXMLElement&);  // Not implemented.
};

#endif
