/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkActorCollection.cxx,v $
  Language:  C++
  Date:      $Date: 2002-05-27 14:27:44 $
  Version:   $Revision: 1.9 $

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActorCollection.h"

#include "vtkObjectFactory.h"
#include "vtkProperty.h"

vtkCxxRevisionMacro(vtkActorCollection, "$Revision: 1.9 $");
vtkStandardNewMacro(vtkActorCollection);

void vtkActorCollection::ApplyProperties(vtkProperty *p)
{
  vtkActor *actor;
  
  if ( p == NULL )
    {
    return;
    }
  
  for ( this->InitTraversal(); (actor=this->GetNextActor()); )
    {
    actor->GetProperty()->DeepCopy(p);
    }
}



