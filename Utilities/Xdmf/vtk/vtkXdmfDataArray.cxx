/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id: vtkXdmfDataArray.cxx,v 1.2 2003-03-04 15:24:32 andy Exp $  */
/*  Date : $Date: 2003-03-04 15:24:32 $ */
/*  Version : $Revision: 1.2 $ */
/*                                                                 */
/*  Author:                                                        */
/*     Jerry A. Clarke                                             */
/*     clarke@arl.army.mil                                         */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2002 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#include <vtkXdmfDataArray.h>

#include <vtkObjectFactory.h>
#include <vtkCommand.h>

#include <vtkUnsignedCharArray.h>
#include <vtkIntArray.h>
#include <vtkFloatArray.h>
#include <vtkDoubleArray.h>

#include <XdmfArray.h>

//----------------------------------------------------------------------------
vtkXdmfDataArray* vtkXdmfDataArray::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkXdmfDataArray");
  if(ret)
    {
    return (vtkXdmfDataArray*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkXdmfDataArray;
}

vtkXdmfDataArray::vtkXdmfDataArray()
{
  this->Array = NULL;
  this->vtkArray = NULL;
}


vtkDataArray *vtkXdmfDataArray::FromXdmfArray( char *ArrayName, int CopyShape ){
  XdmfArray *Array = this->Array;
  if ( ArrayName != NULL ) {
    Array = TagNameToArray( ArrayName );
    }
  if( Array == NULL ){
    XdmfErrorMessage("Array is NULL");
    return(NULL);
    }
  switch( Array->GetNumberType() ){
    case XDMF_INT8_TYPE :
      if( this->vtkArray == NULL ) {
        this->vtkArray = vtkUnsignedCharArray::New();
        }
      break;
    case XDMF_INT32_TYPE :
    case XDMF_INT64_TYPE :
      if( this->vtkArray == NULL ) {
        this->vtkArray = vtkIntArray::New();
        }
      break;
    case XDMF_FLOAT32_TYPE :
      if( this->vtkArray == NULL ) {
        this->vtkArray = vtkFloatArray::New();
        }
      break;
    default :
      if( this->vtkArray == NULL ) {
        this->vtkArray = vtkDoubleArray::New();
        }
      break;
    }
  if ( CopyShape && ( Array->GetRank() == 2 ) ) {
    this->vtkArray->SetNumberOfComponents( Array->GetDimension( 1 ) );
    this->vtkArray->SetNumberOfTuples( Array->GetDimension( 0 ) );
  } else {
    this->vtkArray->SetNumberOfComponents( 1 );
    this->vtkArray->SetNumberOfTuples( Array->GetNumberOfElements() );
    }
  switch( Array->GetNumberType() ){
    case XDMF_INT8_TYPE :
      Array->GetValues( 0,
        ( unsigned char  *)this->vtkArray->GetVoidPointer( 0 ),
        Array->GetNumberOfElements() );  
      break;
    case XDMF_INT32_TYPE :
    case XDMF_INT64_TYPE :
      Array->GetValues( 0,
        ( int *)this->vtkArray->GetVoidPointer( 0 ),
        Array->GetNumberOfElements() );  
      break;
    case XDMF_FLOAT32_TYPE :
      Array->GetValues( 0,
        ( float *)this->vtkArray->GetVoidPointer( 0 ),
        Array->GetNumberOfElements() );  
      break;
    default :
      Array->GetValues( 0,
        ( double *)this->vtkArray->GetVoidPointer( 0 ),
        Array->GetNumberOfElements() );  
      break;
    }
return( this->vtkArray );
}

char *vtkXdmfDataArray::ToXdmfArray( vtkDataArray *DataArray, int CopyShape ){
  XdmfArray *Array;
  if ( DataArray  == NULL )  {
    DataArray = this->vtkArray;
    }
  if ( DataArray  == NULL )  {
    vtkDebugMacro(<< "Array is NULL");
    return(NULL);
    }
  if ( this->Array == NULL ){
    this->Array = new XdmfArray();
    switch( DataArray->GetDataType() ){
      case VTK_CHAR :
      case VTK_UNSIGNED_CHAR :
        this->Array->SetNumberType( XDMF_INT8_TYPE );
        break;
      case VTK_SHORT :
      case VTK_UNSIGNED_SHORT :
      case VTK_INT :
      case VTK_UNSIGNED_INT :
      case VTK_LONG :
      case VTK_UNSIGNED_LONG :
        this->Array->SetNumberType( XDMF_INT32_TYPE );
        break;
      case VTK_FLOAT :
        this->Array->SetNumberType( XDMF_FLOAT32_TYPE );
        break;
      case VTK_DOUBLE :
        this->Array->SetNumberType( XDMF_FLOAT64_TYPE );
        break;
      default :
        XdmfErrorMessage("Can't handle Data Type");
        return( NULL );
      }
    }
  Array = this->Array;
  if( CopyShape ) {
    XdmfInt64 Shape[3];

    Shape[0] = DataArray->GetNumberOfTuples();
    Shape[1] = DataArray->GetNumberOfComponents();
    if( Shape[1] == 1 ) {
      Array->SetShape( 1, Shape );
    } else {
      Array->SetShape( 2, Shape );
    }

    }
  switch( Array->GetNumberType() ){
    case XDMF_INT8_TYPE :
      Array->SetValues( 0,
        ( unsigned char  *)DataArray->GetVoidPointer( 0 ),
        Array->GetNumberOfElements() );  
      break;
    case XDMF_INT32_TYPE :
    case XDMF_INT64_TYPE :
      Array->SetValues( 0,
        ( int *)DataArray->GetVoidPointer( 0 ),
        Array->GetNumberOfElements() );  
      break;
    case XDMF_FLOAT32_TYPE :
      Array->SetValues( 0,
        ( float *)DataArray->GetVoidPointer( 0 ),
        Array->GetNumberOfElements() );  
      break;
    default :
      Array->SetValues( 0,
        ( double *)DataArray->GetVoidPointer( 0 ),
        Array->GetNumberOfElements() );  
      break;
    }
return( Array->GetTagName() );
}
