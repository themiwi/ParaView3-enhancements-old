/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id: XdmfGeometry.cxx,v 1.3 2007-04-10 17:04:56 clarke Exp $  */
/*  Date : $Date: 2007-04-10 17:04:56 $ */
/*  Version : $Revision: 1.3 $ */
/*                                                                 */
/*  Author:                                                        */
/*     Jerry A. Clarke                                             */
/*     clarke@arl.army.mil                                         */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2007 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#include "XdmfGeometry.h"

#include "XdmfTopology.h"
#include "XdmfDataItem.h"
#include "XdmfArray.h"
#include "XdmfDOM.h"
#include "XdmfHDF.h"

XdmfGeometry *GetXdmfGeometryHandle( void *Pointer ){
  //XdmfGeometry *tmp = (XdmfGeometry *)Pointer;
  return((XdmfGeometry *)Pointer);
  }

XdmfGeometry::XdmfGeometry() {
  this->GeometryType = XDMF_GEOMETRY_NONE;
  this->Points = NULL;
  this->PointsAreMine = 1;
  this->VectorX = NULL;
  this->VectorY = NULL;
  this->VectorZ = NULL;
  this->SetOrigin( 0, 0, 0 );
  this->SetDxDyDz( 0, 0, 0 );
  }

XdmfGeometry::~XdmfGeometry() {
  if( this->PointsAreMine && this->Points )  delete this->Points;
  }

XdmfInt32
XdmfGeometry::SetOrigin( XdmfFloat64 X, XdmfFloat64 Y, XdmfFloat64 Z ){

this->Origin[0] = X;
this->Origin[1] = Y;
this->Origin[2] = Z;
return( XDMF_SUCCESS );
}

XdmfInt32
XdmfGeometry::SetOrigin( XdmfFloat64 *origin ){
return( this->SetOrigin( origin[0], origin[1], origin[2] ) );
}

XdmfInt32
XdmfGeometry::SetDxDyDz( XdmfFloat64 Dx, XdmfFloat64 Dy, XdmfFloat64 Dz ){
this->DxDyDz[0] = Dx;
this->DxDyDz[1] = Dy;
this->DxDyDz[2] = Dz;
return( XDMF_SUCCESS );
}


XdmfInt32
XdmfGeometry::SetDxDyDz( XdmfFloat64 *dxDyDz ){
return( this->SetDxDyDz( dxDyDz[0], dxDyDz[1], dxDyDz[2] ) );
}


XdmfInt32
XdmfGeometry::SetPoints( XdmfArray *points ){
    if( this->PointsAreMine && this->Points ) delete points;
    this->PointsAreMine = 0;
    this->Points = points;
    return( XDMF_SUCCESS );
    }

XdmfInt32
XdmfGeometry::SetGeometryTypeFromString( XdmfConstString geometryType ){

if( XDMF_WORD_CMP( geometryType, "X_Y_Z" ) ){
  this->GeometryType = XDMF_GEOMETRY_X_Y_Z;
  return(XDMF_SUCCESS);
  }
if( XDMF_WORD_CMP( geometryType, "X_Y" ) ){
  this->GeometryType = XDMF_GEOMETRY_X_Y;
  return(XDMF_SUCCESS);
  }
if( XDMF_WORD_CMP( geometryType, "XY" ) ){
  this->GeometryType = XDMF_GEOMETRY_XY;
  return(XDMF_SUCCESS);
  }
if( XDMF_WORD_CMP( geometryType, "XYZ" ) ){
  this->GeometryType = XDMF_GEOMETRY_XYZ;
  return(XDMF_SUCCESS);
  }
if( XDMF_WORD_CMP( geometryType, "ORIGIN_DXDYDZ" ) ){
  this->GeometryType = XDMF_GEOMETRY_ORIGIN_DXDYDZ;
  return(XDMF_SUCCESS);
  }
if( XDMF_WORD_CMP( geometryType, "VXVYVZ" ) ){
  this->GeometryType = XDMF_GEOMETRY_VXVYVZ;
  return(XDMF_SUCCESS);
  }
return( XDMF_FAIL );
}

XdmfString
XdmfGeometry::GetGeometryTypeAsString( void ){
static char Value[80];

switch( this->GeometryType ){
  case XDMF_GEOMETRY_VXVYVZ:
    strcpy( Value, "VXVYVZ" );
    break;
  case XDMF_GEOMETRY_ORIGIN_DXDYDZ:
    strcpy( Value, "ORIGIN_DXDYDZ" );
    break;
  case XDMF_GEOMETRY_X_Y_Z :
    strcpy( Value, "X_Y_Z" );
    break;
  case XDMF_GEOMETRY_X_Y :
    strcpy( Value, "X_Y" );
    break;
  case XDMF_GEOMETRY_XY :
    strcpy( Value, "XY" );
    break;
  default :
    strcpy( Value, "XYZ" );
    break;
  }
return( Value );
}

XdmfInt32
XdmfGeometry::UpdateInformation() {
XdmfConstString  Attribute;

if(XdmfElement::UpdateInformation() != XDMF_SUCCESS) return(XDMF_FAIL);
if( XDMF_WORD_CMP(this->GetElementType(), "Geometry") == 0){
    XdmfErrorMessage("Element type" << this->GetElementType() << " is not of type 'Geometry'");
    return(XDMF_FAIL);
}
Attribute = this->Get( "Type" );
if( Attribute ){
  this->SetGeometryTypeFromString( Attribute );
} else {
  this->GeometryType = XDMF_GEOMETRY_XYZ;
}
if(!this->Name) this->SetName(GetUnique("Geometry_"));
return( XDMF_SUCCESS );
}

XdmfInt32
XdmfGeometry::Update() {

XdmfConstString  Attribute;
XdmfInt32  ArrayIndex;
XdmfInt64  Start[ XDMF_MAX_DIMENSION ];
XdmfInt64  Stride[ XDMF_MAX_DIMENSION ];
XdmfInt64  Count[ XDMF_MAX_DIMENSION ];
XdmfXmlNode     PointsElement;
XdmfArray       *points = NULL;
XdmfArray       *TmpArray;

if( this->GeometryType == XDMF_GEOMETRY_NONE ){
  if( this->UpdateInformation() == XDMF_FAIL ){
    XdmfErrorMessage("Can't Initialize From Element");
    return( XDMF_FAIL );
  }
}
if(XdmfElement::Update() != XDMF_SUCCESS) return(XDMF_FAIL);
ArrayIndex = 0;
if( ( this->GeometryType == XDMF_GEOMETRY_X_Y_Z ) ||
  ( this->GeometryType == XDMF_GEOMETRY_X_Y ) ||
  ( this->GeometryType == XDMF_GEOMETRY_XYZ ) ||
  ( this->GeometryType == XDMF_GEOMETRY_XY ) ){
 do {
  // Read the Data
  XdmfDebug("Reading Points ( SubElement #" << ArrayIndex + 1 << " )" );
  PointsElement = this->DOM->FindDataElement( ArrayIndex, Element );
  if( PointsElement ){
    XdmfDataItem PointsItem;
    if(PointsItem.SetDOM( this->DOM ) == XDMF_FAIL) return(XDMF_FAIL);
    if(PointsItem.SetElement(PointsElement) == XDMF_FAIL) return(XDMF_FAIL);
    if(PointsItem.UpdateInformation() == XDMF_FAIL) return(XDMF_FAIL);
    if(PointsItem.Update() == XDMF_FAIL) return(XDMF_FAIL);
    TmpArray = PointsItem.GetArray();
    if( TmpArray ){
        if( !points ){
            switch( this->GeometryType ){
                case XDMF_GEOMETRY_X_Y_Z :
                    points = new XdmfArray;
                    points->CopyType( TmpArray );
                    points->SetNumberOfElements( TmpArray->GetNumberOfElements() * 3 );
                    break;
                case XDMF_GEOMETRY_XY :
                    points = new XdmfArray;
                    points->CopyType( TmpArray );
                    points->SetNumberOfElements( TmpArray->GetNumberOfElements() / 2 * 3 );
                    break;
                case XDMF_GEOMETRY_X_Y :
                    points = new XdmfArray;
                    points->CopyType( TmpArray );
                    points->SetNumberOfElements( TmpArray->GetNumberOfElements() * 3 );
                    break;
                default :
                    points = TmpArray;
                    // Assure DataItem Destructor does not delete XdmfArray
                    PointsItem.SetArrayIsMine(0);
                    break;
            }
        }
        // We Have made Points Rank = 1 if not XYZ
        switch( this->GeometryType ){
            case XDMF_GEOMETRY_X_Y_Z :
                    Start[0] = ArrayIndex;
                    Stride[0] = 3;
                    points->SelectHyperSlab( Start, Stride, NULL );
                    CopyArray( TmpArray, points);
                    this->NumberOfPoints = TmpArray->GetNumberOfElements();
                    break;
            case XDMF_GEOMETRY_XY :
                    Start[0] = TmpArray->GetNumberOfElements();
                    Start[1] = 3;
                    points->SetShape( 2 , Start );
                    Stride[0] = 1;
                    Stride[0] = 1;
                    Count[0] = TmpArray->GetNumberOfElements();
                    Count[1] = 2;
                    points->SelectHyperSlab( NULL, Stride, Count);
                    CopyArray( TmpArray, points);
                    this->NumberOfPoints = TmpArray->GetNumberOfElements() / 2 ;
                    break;
            case XDMF_GEOMETRY_X_Y :
                    Start[0] = ArrayIndex;
                    Stride[0] = 3;
                    points->SelectHyperSlab( Start, Stride, NULL );
                    CopyArray( TmpArray, points);
                    this->NumberOfPoints = TmpArray->GetNumberOfElements();
                    break;
            default :
                    // points = TmpArray so do nothing
                    this->NumberOfPoints = TmpArray->GetNumberOfElements() / 3;
                    break;
        }
    }
  } 
  ArrayIndex++;
 } while( ( ArrayIndex < 3 ) && ( PointsElement != NULL ) );
} else {
  if( this->GeometryType == XDMF_GEOMETRY_ORIGIN_DXDYDZ ) {
      XdmfDataItem PointsItem;
      PointsItem.SetDOM(this->DOM);
      XdmfDebug("Reading Origin and Dx, Dy, Dz" );
      PointsElement = this->DOM->FindDataElement(0, this->Element );
      if( PointsElement ){
        if(PointsItem.SetElement(PointsElement) == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.UpdateInformation() == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.Update() == XDMF_FAIL) return(XDMF_FAIL);
        TmpArray = PointsItem.GetArray();
        if( TmpArray ){
            TmpArray->GetValues( 0, this->Origin, 3 );
        }
      PointsElement = this->DOM->FindDataElement(1, this->Element );
      if( PointsElement ){
        if(PointsItem.SetElement(PointsElement) == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.UpdateInformation() == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.Update() == XDMF_FAIL) return(XDMF_FAIL);
        TmpArray = PointsItem.GetArray();
        if( TmpArray ){
          TmpArray->GetValues( 0, this->DxDyDz, 3 );
        }
      } else {
        XdmfErrorMessage("No Dx, Dy, Dz Specified");
        return( XDMF_FAIL );
      }
    } else {
      XdmfErrorMessage("No Origin Specified");
      return( XDMF_FAIL );
    }
  } else if( this->GeometryType == XDMF_GEOMETRY_VXVYVZ ) {
      XdmfDebug("Reading VectorX, VectorY, VectorZ" );
      PointsElement = this->DOM->FindDataElement(0, this->Element );
      if( PointsElement ){
        XdmfDataItem PointsItem;
        PointsItem.SetDOM(this->DOM);
        if(PointsItem.SetElement(PointsElement) == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.UpdateInformation() == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.Update() == XDMF_FAIL) return(XDMF_FAIL);
        TmpArray = PointsItem.GetArray();
        if(!TmpArray){
            XdmfErrorMessage("Error Reading Points X Vector");
            return(XDMF_FAIL);
        }
        this->VectorX = TmpArray;
        PointsItem.SetArrayIsMine(0);
    } else {
      XdmfErrorMessage("No VectorX Specified");
      return( XDMF_FAIL );
      }
      PointsElement = this->DOM->FindDataElement(1, this->Element );
      if( PointsElement ){
        XdmfDataItem PointsItem;
        PointsItem.SetDOM(this->DOM);
        if(PointsItem.SetElement(PointsElement) == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.UpdateInformation() == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.Update() == XDMF_FAIL) return(XDMF_FAIL);
        TmpArray = PointsItem.GetArray();
        if(!TmpArray){
            XdmfErrorMessage("Error Reading Points Y Vector");
            return(XDMF_FAIL);
        }
        this->VectorY = TmpArray;
        PointsItem.SetArrayIsMine(0);
    } else {
      XdmfErrorMessage("No VectorY Specified");
      return( XDMF_FAIL );
      }
      PointsElement = this->DOM->FindDataElement(2, this->Element );
      if( PointsElement ){
        XdmfDataItem PointsItem;
        PointsItem.SetDOM(this->DOM);
        if(PointsItem.SetElement(PointsElement) == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.UpdateInformation() == XDMF_FAIL) return(XDMF_FAIL);
        if(PointsItem.Update() == XDMF_FAIL) return(XDMF_FAIL);
        TmpArray = PointsItem.GetArray();
        if(!TmpArray){
            XdmfErrorMessage("Error Reading Points Z Vector");
            return(XDMF_FAIL);
        }
        this->VectorZ = TmpArray;
        PointsItem.SetArrayIsMine(0);
    } else {
      XdmfErrorMessage("No VectorZ Specified");
      return( XDMF_FAIL );
      }
  }
}
if( points ) this->SetPoints( points );
return( XDMF_SUCCESS );
}
