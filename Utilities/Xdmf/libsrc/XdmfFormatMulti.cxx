/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id: XdmfFormatMulti.cxx,v 1.4 2003-10-21 15:12:48 andy Exp $  */
/*  Date : $Date: 2003-10-21 15:12:48 $ */
/*  Version : $Revision: 1.4 $ */
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
#include "XdmfFormatMulti.h"

#include "XdmfDOM.h"
#include "XdmfFormatHDF.h"
#include "XdmfFormatXML.h"

XdmfFormatMulti::XdmfFormatMulti() {
  strcpy( this->DataFormat, "HDF" );
}

XdmfFormatMulti::~XdmfFormatMulti() {
}

XdmfArray *
XdmfFormatMulti::ElementToArray( XdmfXNode *Element, XdmfDataDesc *Desc, XdmfArray *Array  ) {
XdmfConstString  ElementType;
XdmfConstString  ElementFormat;

if( !Element ){
  XdmfErrorMessage("Element is NULL");
  return( NULL );
  }
ElementType = this->DOM->Get(Element, "NodeType");
if( XDMF_WORD_CMP(ElementType, "DataStructure") == 0  ){
  Element = this->DOM->FindElement("DataStructure", 0, Element );
  if(!Element){
    XdmfErrorMessage("Can't Find Element of Propoer Type");
    return( NULL );
    }
}
ElementType = this->DOM->Get(Element, "NodeType");
if( (ElementFormat = this->DOM->Get(Element, "Format")) == NULL ){
  ElementFormat = this->DataFormat;
  }
if(strcmp(ElementFormat, "HDF") == 0 ){
  XdmfFormatHDF  Formatter;
  Formatter.SetDOM( this->GetDOM() );
  Array = Formatter.ElementToArray( Element, Desc, Array );
} else
if(strcmp(ElementFormat, "XML") == 0 ){
  XdmfFormatXML  Formatter;
  Formatter.SetDOM( this->GetDOM() );
  Array = Formatter.ElementToArray( Element, Desc, Array );
} else {
  XdmfErrorMessage("Unknown Format " << ElementFormat );
  return( NULL );
}
return( Array );
}

XdmfXNode *
XdmfFormatMulti::ArrayToElement( XdmfArray *Array,
      XdmfString HeavyDataset,
      XdmfXNode *Element,
      XdmfDataDesc *Desc ) {
XdmfConstString  ElementFormat;

if( Element ){
  ElementFormat = this->DOM->Get( Element, "Format");
  if( !ElementFormat ){
    ElementFormat = this->DataFormat;
  }
} else {
  ElementFormat = this->DataFormat;
}
if(strcmp(ElementFormat, "HDF") == 0 ){
  XdmfFormatHDF  Formatter;
  Formatter.SetDOM( this->GetDOM() );
  Element = Formatter.ArrayToElement( Array, HeavyDataset, Element, Desc );
} else
if(strcmp(ElementFormat, "XML") == 0 ){
  XdmfFormatXML  Formatter;
  Formatter.SetDOM( this->GetDOM() );
  Element = Formatter.ArrayToElement( Array, HeavyDataset, Element, Desc );
} else {
  XdmfErrorMessage("Unknown Format " << ElementFormat );
  return( NULL );
}
return( Element );
}

XdmfConstString
XdmfFormatMulti::SetFormatFromElement( XdmfXNode *Element ) {
XdmfConstString  Format;

Format = this->DOM->Get(Element, "Format");
if( Format ) {
  strcpy( this->DataFormat, Format );
  }
return( Format );
}

