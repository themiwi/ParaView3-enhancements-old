/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkInformation.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformation.h"

#include "vtkCommand.h"
#include "vtkGarbageCollector.h"
#include "vtkInformationDataObjectKey.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIdTypeKey.h"
#include "vtkInformationInformationKey.h"
#include "vtkInformationInformationVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerPointerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationIterator.h"
#include "vtkInformationKeyVectorKey.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationRequestKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationStringVectorKey.h"
#include "vtkInformationUnsignedLongKey.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <vtkstd/algorithm>
#include <vtkstd/utility>
#include <vtkstd/vector>

#include "vtkInformationInternals.h"

vtkCxxRevisionMacro(vtkInformation, "$Revision: 1.31 $");
vtkStandardNewMacro(vtkInformation);

//----------------------------------------------------------------------------
vtkInformation::vtkInformation()
{
  // Allocate the internal representation.
  this->Internal = new vtkInformationInternals;

  // There is no request key stored initially.
  this->Request = 0;
}

//----------------------------------------------------------------------------
vtkInformation::~vtkInformation()
{
  // Delete the internal representation.
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  // Print the request if one is set.
  if(this->Request)
    {
    os << indent << "Request: " << this->Request->GetName() << "\n";
    }

  // Give each key a chance to print its value.
  unsigned short i;
  for (i = 0; i < this->Internal->TableSize; ++i)
    {
    if (this->Internal->Keys[i])
      {
      // Print the key name first.
      vtkInformationKey* key = this->Internal->Keys[i];
      os << indent << key->GetName() << ": ";
      
      // Ask the key to print its value.
      key->Print(os, this);
      os << "\n";
      }
    }
}
  
//----------------------------------------------------------------------------
void vtkInformation::PrintKeys(ostream& os, vtkIndent indent)
{
  // Give each key a chance to print its value.
  unsigned short i;
  for (i = 0; i < this->Internal->TableSize; ++i)
    {
    if (this->Internal->Keys[i])
      {
      // Print the key name first.
      vtkInformationKey* key = this->Internal->Keys[i];
      os << indent << key->GetName() << ": ";
      // Ask the key to print its value.
      key->Print(os, this);
      os << "\n";
      }
    }
}


//----------------------------------------------------------------------------
// call modified on superclass
void vtkInformation::Modified()
{
  this->Superclass::Modified();
}

//----------------------------------------------------------------------------
// Update MTime and invoke a modified event with
// the information key as call data
void vtkInformation::Modified(vtkInformationKey* key)
{
  this->MTime.Modified();
  this->InvokeEvent(vtkCommand::ModifiedEvent, key);
}

//----------------------------------------------------------------------------
// Return the number of keys as a result of iteration.
int vtkInformation::GetNumberOfKeys()
{
  vtkSmartPointer<vtkInformationIterator> infoIterator = 
    vtkSmartPointer<vtkInformationIterator>::New();
  infoIterator->SetInformation( this );

  int numberOfKeys = 0;
  for (infoIterator->InitTraversal(); !infoIterator->IsDoneWithTraversal();
    infoIterator->GoToNextItem())
    {
    numberOfKeys++;
    }
  return numberOfKeys;
}

//----------------------------------------------------------------------------
// grow the table by a factor of 2
void vtkInformation::ExpandTable()
{
  vtkInformationInternals* oldInternal = this->Internal;
  this->Internal = new vtkInformationInternals(
    static_cast<int>(oldInternal->TableSize*2.2));
  
  unsigned short i;
  for (i = 0; i < oldInternal->TableSize; ++i)
    {
    if (oldInternal->Keys[i])
      {
      this->SetAsObjectBase(oldInternal->Keys[i],oldInternal->Values[i]);
      }
    }
  delete oldInternal;
}


//----------------------------------------------------------------------------
void vtkInformation::SetAsObjectBase(vtkInformationKey* key,
                                     vtkObjectBase* newvalue)
{
  if(!key)
    {
    return;
    }
  
  // compute the hash
  unsigned short ohash = this->Internal->Hash(reinterpret_cast<unsigned long>(key));
  unsigned short hash = ohash;
  
  // Check for an existing entry.
  vtkInformationKey *val = this->Internal->Keys[hash];
  // is there something in this hash slot
  if (val)
    {
    while (val && val != key && hash < this->Internal->TableSize -1)
      {
      hash++;
      val = this->Internal->Keys[hash];
      }
    // if we have exceeded the table size or have two collisions
    if ((hash == this->Internal->TableSize -1  && val != key) || hash - ohash > 1)
      {
      this->ExpandTable();
      this->SetAsObjectBase(key,newvalue);
      return;
      }
    // if there is an entry for this key
    if(val)
      {
      // Update the value.
      vtkObjectBase* oldvalue = this->Internal->Values[hash];
      if(newvalue)
        {
        // There is a new value.  Replace the entry.
        this->Internal->Values[hash] = newvalue;
        newvalue->Register(0);
        }
      // remove the value
      else
        {
        // There is no new value.  Erase the entry.  and shift down any
        // followup entries that hash to the same value, requires that they
        // be sorted
        hash++;
        while (hash < this->Internal->TableSize && 
               this->Internal->Keys[hash] && 
               this->Internal->Hash(reinterpret_cast<unsigned long>
                                    (this->Internal->Keys[hash])) < hash)
          {
          this->Internal->Keys[hash-1] = this->Internal->Keys[hash];
          this->Internal->Values[hash-1] = this->Internal->Values[hash];
          hash++;
          }
        // clear the final entry 
        this->Internal->Keys[hash-1] = 0;
        }
      oldvalue->UnRegister(0);
      }
    // add an entry but after the desired hash location
    else if (newvalue)
      {
      // start at ohash + 1 and find where we should instert this key
      unsigned short hash2 = hash;
      hash = ohash + 1;
      while (this->Internal->Hash(reinterpret_cast<unsigned long>
                                  (this->Internal->Keys[hash])) == ohash)
        {
        hash++;
        }
      // insert and shift the rest, hash is the insertion point
      for (;hash2 > hash; --hash2)
        {
        this->Internal->Keys[hash2] = this->Internal->Keys[hash2-1];
        this->Internal->Values[hash2] = this->Internal->Values[hash2-1];
        }
      this->Internal->Keys[hash2] = key;
      this->Internal->Values[hash2] = newvalue;      
      newvalue->Register(0);
      }
    }
  else if (newvalue)
    {
    // There is no entry with this key.  Create one and store the value.
    newvalue->Register(0);
    this->Internal->Keys[hash] = key;
    this->Internal->Values[hash] = newvalue;
    }
  
  this->Modified(key);
}

//----------------------------------------------------------------------------
vtkObjectBase* vtkInformation::GetAsObjectBase(vtkInformationKey* key)
{
  if(key)
    {
    // compute the hash
    unsigned short hash = 
      this->Internal->Hash(reinterpret_cast<unsigned long>(key));
    
    // Check for an existing entry.
    vtkInformationKey *val = this->Internal->Keys[hash];
        while (hash < this->Internal->TableSize - 1 && val && val != key)
      {
      hash++;
      val = this->Internal->Keys[hash];
      }
    if (val == key)
      {
      return this->Internal->Values[hash];
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkInformation::Clear()
{
  this->Copy(0);
}

//----------------------------------------------------------------------------
void vtkInformation::Copy(vtkInformation* from, int deep)
{
  vtkInformationInternals* oldInternal = this->Internal;
  if(from)
    {
    this->Internal = new vtkInformationInternals(from->Internal->TableSize);
    unsigned short i;
    for (i = 0; i < from->Internal->TableSize; ++i)
      {
      if (from->Internal->Keys[i])
        {
        this->CopyEntry(from, from->Internal->Keys[i], deep);
        }
      }
    }
  else
    {
    this->Internal = new vtkInformationInternals;
    }
  delete oldInternal;
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, 
                               vtkInformationKey* key, int deep)
{
  if (!deep)
    {
    key->ShallowCopy(from, this);
    }
  else
    {
    key->DeepCopy(from, this);
    }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, 
                               vtkInformationDataObjectKey* key, int deep)
{
  if (!deep)
    {
    key->ShallowCopy(from, this);
    }
  else
    {
    key->DeepCopy(from, this);
    }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, 
                               vtkInformationInformationKey* key, int deep)
{
  if (!deep)
    {
    key->ShallowCopy(from, this);
    }
  else
    {
    key->DeepCopy(from, this);
    }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, 
                               vtkInformationInformationVectorKey* key, 
                               int deep)
{
  if (!deep)
    {
    key->ShallowCopy(from, this);
    }
  else
    {
    key->DeepCopy(from, this);
    }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, 
                               vtkInformationIntegerKey* key, int deep)
{
  if (!deep)
    {
    key->ShallowCopy(from, this);
    }
  else
    {
    key->DeepCopy(from, this);
    }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, 
                               vtkInformationRequestKey* key, int deep)
{
  if (!deep)
    {
    key->ShallowCopy(from, this);
    }
  else
    {
    key->DeepCopy(from, this);
    }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, 
                               vtkInformationIntegerVectorKey* key, int deep)
{
  if (!deep)
    {
    key->ShallowCopy(from, this);
    }
  else
    {
    key->DeepCopy(from, this);
    }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, 
                               vtkInformationDoubleVectorKey* key, int deep)
{
  if (!deep)
    {
    key->ShallowCopy(from, this);
    }
  else
    {
    key->DeepCopy(from, this);
    }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, 
                               vtkInformationStringKey* key, int deep)
{
  if (!deep)
    {
    key->ShallowCopy(from, this);
    }
  else
    {
    key->DeepCopy(from, this);
    }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, 
                               vtkInformationUnsignedLongKey* key, int deep)
{
  if (!deep)
    {
    key->ShallowCopy(from, this);
    }
  else
    {
    key->DeepCopy(from, this);
    }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntry(vtkInformation* from, 
                               vtkInformationStringVectorKey* key, int deep)
{
  if (!deep)
    {
    key->ShallowCopy(from, this);
    }
  else
    {
    key->DeepCopy(from, this);
    }
}

//----------------------------------------------------------------------------
void vtkInformation::CopyEntries(vtkInformation* from,
                                 vtkInformationKeyVectorKey* key, int deep)
{
  int numberOfKeys = from->Length(key);
  vtkInformationKey** keys = from->Get(key);
  for(int i=0; i < numberOfKeys; ++i)
    {
    this->CopyEntry(from, keys[i], deep);
    }
}

//----------------------------------------------------------------------------
int vtkInformation::Has(vtkInformationKey* key)
{
  // Use the virtual interface in case this is a special-cased key.
  return key->Has(this)?1:0;
}

//----------------------------------------------------------------------------
void vtkInformation::Remove(vtkInformationKey* key)
{
  // Use the virtual interface in case this is a special-cased key.
  key->Remove(this);
}

void vtkInformation::Set(vtkInformationRequestKey* key)
{
  key->Set(this);
}
void vtkInformation::Remove(vtkInformationRequestKey* key)
{
  key->vtkInformationRequestKey::Remove(this);
}
int vtkInformation::Has(vtkInformationRequestKey* key)
{
  return key->vtkInformationRequestKey::Has(this);
}

//----------------------------------------------------------------------------
#define VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(name, type)                  \
  void vtkInformation::Set(vtkInformation##name##Key* key, type value)      \
    {                                                                       \
    key->Set(this, value);                                                  \
    }                                                                       \
  void vtkInformation::Remove(vtkInformation##name##Key* key)               \
    {                                                                       \
    key->vtkInformation##name##Key::Remove(this);                           \
    }                                                                       \
  type vtkInformation::Get(vtkInformation##name##Key* key)                  \
    {                                                                       \
    return key->Get(this);                                                  \
    }                                                                       \
  int vtkInformation::Has(vtkInformation##name##Key* key)                   \
    {                                                                       \
    return key->vtkInformation##name##Key::Has(this);                       \
    }
VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(IdType, vtkIdType);
VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(Integer, int);
VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(Double, double);
VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(UnsignedLong, unsigned long);
VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(String, const char*);
VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(DataObject, vtkDataObject*);
VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(Information, vtkInformation*);
VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(InformationVector, vtkInformationVector*);
VTK_INFORMATION_DEFINE_SCALAR_PROPERTY(ObjectBase, vtkObjectBase*);
#undef VTK_INFORMATION_DEFINE_SCALAR_PROPERTY

//----------------------------------------------------------------------------
#define VTK_INFORMATION_DEFINE_VECTOR_PROPERTY(name, type)                  \
  void vtkInformation::Append(vtkInformation##name##VectorKey* key,         \
                              type value)                                   \
    {                                                                       \
    key->Append(this, value);                                               \
    }                                                                       \
  void vtkInformation::Set(vtkInformation##name##VectorKey* key,            \
                           type* value, int length)                         \
    {                                                                       \
    key->Set(this, value, length);                                          \
    }                                                                       \
  type* vtkInformation::Get(vtkInformation##name##VectorKey* key)           \
    {                                                                       \
    return key->Get(this);                                                  \
    }                                                                       \
  type vtkInformation::Get(vtkInformation##name##VectorKey* key, int idx)   \
    {                                                                       \
    return key->Get(this, idx);                                             \
    }                                                                       \
  void vtkInformation::Get(vtkInformation##name##VectorKey* key,            \
                           type* value)                                     \
    {                                                                       \
    key->Get(this, value);                                                  \
    }                                                                       \
  int vtkInformation::Length(vtkInformation##name##VectorKey* key)          \
    {                                                                       \
    return key->Length(this);                                               \
    }                                                                       \
  void vtkInformation::Remove(vtkInformation##name##VectorKey* key)         \
    {                                                                       \
    key->vtkInformation##name##VectorKey::Remove(this);                     \
    }                                                                       \
  int vtkInformation::Has(vtkInformation##name##VectorKey* key)             \
    {                                                                       \
    return key->vtkInformation##name##VectorKey::Has(this);                 \
    }
VTK_INFORMATION_DEFINE_VECTOR_PROPERTY(Integer, int);
VTK_INFORMATION_DEFINE_VECTOR_PROPERTY(Double, double);

// String vector key is slightly different to make it backwards compatible with
// the scalar string key.
void vtkInformation::Append(vtkInformationStringVectorKey* key,
                            const char* value)
  {
  key->Append(this, value);
  }
void vtkInformation::Set(vtkInformationStringVectorKey* key,
    const char* value, int idx)
  {
  key->Set(this, value, idx);
  }
const char* vtkInformation::Get(vtkInformationStringVectorKey* key, int idx)
  {
  return key->Get(this, idx);
  }
int vtkInformation::Length(vtkInformationStringVectorKey* key)
  {
  return key->Length(this);
  }
void vtkInformation::Remove(vtkInformationStringVectorKey* key)
  {
  key->vtkInformationStringVectorKey::Remove(this);
  }
int vtkInformation::Has(vtkInformationStringVectorKey* key)
  {
  return key->vtkInformationStringVectorKey::Has(this);
  }

VTK_INFORMATION_DEFINE_VECTOR_PROPERTY(Key, vtkInformationKey*);
#define VTK_INFORMATION_DEFINE_VECTOR_VALUE_PROPERTY(name, type)            \
  void vtkInformation::Set(vtkInformation##name##VectorKey* key,            \
                           type value1, type value2, type value3,           \
                           type value4, type value5, type value6)           \
    {                                                                       \
    type value[6];                                                          \
    value[0] = value1;                                                      \
    value[1] = value2;                                                      \
    value[2] = value3;                                                      \
    value[3] = value4;                                                      \
    value[4] = value5;                                                      \
    value[5] = value6;                                                      \
    key->Set(this, value, 6);                                               \
    }                                                                       \
  void vtkInformation::Set(vtkInformation##name##VectorKey* key,            \
                           type value1, type value2, type value3)           \
    {                                                                       \
    type value[3];                                                          \
    value[0] = value1;                                                      \
    value[1] = value2;                                                      \
    value[2] = value3;                                                      \
    key->Set(this, value, 3);                                               \
    }
VTK_INFORMATION_DEFINE_VECTOR_VALUE_PROPERTY(Integer, int);
VTK_INFORMATION_DEFINE_VECTOR_VALUE_PROPERTY(Double, double);
#undef VTK_INFORMATION_DEFINE_VECTOR_VALUE_PROPERTY

#undef VTK_INFORMATION_DEFINE_VECTOR_PROPERTY

//----------------------------------------------------------------------------
#define VTK_INFORMATION_DEFINE_POINTER_PROPERTY(name, type)                  \
  void vtkInformation::Set(vtkInformation##name##PointerKey* key,            \
                           type* value, int length)                         \
    {                                                                       \
    key->Set(this, value, length);                                          \
    }                                                                       \
  type* vtkInformation::Get(vtkInformation##name##PointerKey* key)           \
    {                                                                       \
    return key->Get(this);                                                  \
    }                                                                       \
  void vtkInformation::Get(vtkInformation##name##PointerKey* key,            \
                           type* value)                                     \
    {                                                                       \
    key->Get(this, value);                                                  \
    }                                                                       \
  int vtkInformation::Length(vtkInformation##name##PointerKey* key)          \
    {                                                                       \
    return key->Length(this);                                               \
    }                                                                       \
  void vtkInformation::Remove(vtkInformation##name##PointerKey* key)         \
    {                                                                       \
    key->vtkInformation##name##PointerKey::Remove(this);                    \
    }                                                                       \
  int vtkInformation::Has(vtkInformation##name##PointerKey* key)             \
    {                                                                       \
    return key->vtkInformation##name##PointerKey::Has(this);                \
    }
VTK_INFORMATION_DEFINE_POINTER_PROPERTY(Integer, int);
#undef VTK_INFORMATION_DEFINE_POINTER_PROPERTY

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationDataObjectKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationDoubleKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationDoubleVectorKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationInformationKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Remove(vtkInformationKeyVectorKey* key,
                            vtkInformationKey* value)
{
  key->RemoveItem(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationInformationVectorKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationIntegerKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationIntegerVectorKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationStringKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationUnsignedLongKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationObjectBaseKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::Append(vtkInformationKeyVectorKey* key,
                            vtkInformationStringVectorKey* value)
{
  key->Append(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationDataObjectKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationDoubleKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationDoubleVectorKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationInformationKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationInformationVectorKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationIntegerKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationIntegerVectorKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationStringKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationUnsignedLongKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationObjectBaseKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
void vtkInformation::AppendUnique(vtkInformationKeyVectorKey* key,
                                  vtkInformationStringVectorKey* value)
{
  key->AppendUnique(this, value);
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationDataObjectKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationInformationKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationInformationVectorKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationIntegerKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationRequestKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationDoubleKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationIntegerVectorKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationDoubleVectorKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationStringKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationUnsignedLongKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
vtkInformationKey* vtkInformation::GetKey(vtkInformationStringVectorKey* key)
{
  return key;
}

//----------------------------------------------------------------------------
void vtkInformation::Register(vtkObjectBase* o)
{
  this->RegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
void vtkInformation::UnRegister(vtkObjectBase* o)
{
  this->UnRegisterInternal(o, 1);
}

//----------------------------------------------------------------------------
void vtkInformation::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);

  // Ask each key/value pair to report any references it holds.
  unsigned short i;
  for (i = 0; i < this->Internal->TableSize; ++i)
    {
    if (this->Internal->Keys[i])
      {
      this->Internal->Keys[i]->Report(this,collector);
      }
    }
}

//----------------------------------------------------------------------------
void vtkInformation::ReportAsObjectBase(vtkInformationKey* key,
                                        vtkGarbageCollector* collector)
{
  if(key)
    {
    unsigned short ohash = 
      this->Internal->Hash(reinterpret_cast<unsigned long>(key));
    while (this->Internal->Keys[ohash] && 
           this->Internal->Keys[ohash] != key && 
           ohash < this->Internal->TableSize)
      {
      ohash++;
      }
    if (this->Internal->Keys[ohash] && ohash < this->Internal->TableSize)
      {
      vtkGarbageCollectorReport(collector, this->Internal->Values[ohash], 
                                key->GetName());
      return;
      }
    }
}

//----------------------------------------------------------------------------
void vtkInformation::SetRequest(vtkInformationRequestKey* request)
{
  this->Request = request;
}

//----------------------------------------------------------------------------
vtkInformationRequestKey* vtkInformation::GetRequest()
{
  return this->Request;
}
