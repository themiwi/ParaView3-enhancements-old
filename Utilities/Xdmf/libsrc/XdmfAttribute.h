/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id: XdmfAttribute.h,v 1.7 2004-08-19 21:22:35 andy Exp $  */
/*  Date : $Date: 2004-08-19 21:22:35 $ */
/*  Version : $Revision: 1.7 $ */
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
#ifndef __XdmfAttribute_h
#define __XdmfAttribute_h

#include "XdmfLightData.h"

// Value Types
#define XDMF_ATTRIBUTE_TYPE_NONE  0
#define XDMF_ATTRIBUTE_TYPE_SCALAR  1
#define XDMF_ATTRIBUTE_TYPE_VECTOR  2
#define XDMF_ATTRIBUTE_TYPE_TENSOR  3
#define XDMF_ATTRIBUTE_TYPE_MATRIX  4

// Where Values are Assigned
#define XDMF_ATTRIBUTE_CENTER_GRID  0
#define XDMF_ATTRIBUTE_CENTER_CELL  1
#define XDMF_ATTRIBUTE_CENTER_FACE  2
#define XDMF_ATTRIBUTE_CENTER_EDGE  3
#define XDMF_ATTRIBUTE_CENTER_NODE  4

class XdmfTopology;
class XdmfDataDesc;
class XdmfArray;

//! Class for Scalar, Vector, and Tensor Computed Data
/*!
	XdmfAttribute is a Class that handles the Computed Values
	on an XdmfGrid. Values can be Scalar(1), Vector(3), Tensor(9)
	or Matrix(NxM). They may be centered on the Node, Edge, Face,
	Cell, or Grid. 
*/

class XDMF_EXPORT XdmfAttribute : public XdmfLightData {

public:
  XdmfAttribute();
  ~XdmfAttribute();

  XdmfConstString GetClassName() { return ( "XdmfAttribute" ) ; };

//! Set Type
/*!
	Set the Type of the Attribute

	\param AttributeType = XDMF_ATTRIBUTE_TYPE_SCALAR |  XDMF_ATTRIBUTE_TYPE_VECTOR | XDMF_ATTRIBUTE_TYPE_TENSOR | XDMF_ATTRIBUTE_TYPE_MATRIX
*/
  XdmfSetValueMacro( AttributeType, XdmfInt32 );
//! Return the Attribute Type
  XdmfGetValueMacro( AttributeType, XdmfInt32 );

//! Return the if the Attribute is Active
  XdmfGetValueMacro( Active, XdmfInt32 );

//! Set the type using a String
  XdmfInt32 SetAttributeTypeFromString( XdmfConstString AttributeType );
//! Get the Type as a String
  XdmfConstString GetAttributeTypeAsString( void );

  XdmfInt32 SetAttributeCenterFromString( XdmfConstString AttributeCenter );
  XdmfConstString GetAttributeCenterAsString( void );

//! Set the Center
/*!
	Set where the Attribute is centered
	\param XDMF_ATTRIBUTE_CENTER_GRID | XDMF_ATTRIBUTE_CENTER_CELL | XDMF_ATTRIBUTE_CENTER_FACE | XDMF_ATTRIBUTE_CENTER_EDGE | XDMF_ATTRIBUTE_CENTER_NODE

*/
  XdmfSetValueMacro( AttributeCenter, XdmfInt32 );
//! Returns the Center of the Attribute
  XdmfGetValueMacro( AttributeCenter, XdmfInt32 );

//! Returns the Shape of the attribute
  XdmfDataDesc *GetShapeDesc( void ) { return( this->ShapeDesc ); };

//! Sets the values for the Attribute
  XdmfSetValueMacro( Values, XdmfArray *);
//! Retreives the Values of the Attribute
  XdmfGetValueMacro( Values, XdmfArray *);

//! Initialize but don't read the Heavy Data
  XdmfInt32 InitAttributeFromElement( XdmfXNode *Element );
//! Initialize and Read the Heavy Data
  XdmfInt32 SetAttributeFromElement( XdmfXNode *Element );

//  XdmfInt32 SetBaseAttribute( XdmfTopology *Topology, XdmfPointer Grid );

//! Update values from Heavy Data
  XdmfInt32 Update();

protected:

  XdmfInt32  AttributeType;
  XdmfInt32  AttributeCenter;
  XdmfDataDesc  *ShapeDesc;
  XdmfInt32  ValuesAreMine;
  XdmfArray  *Values;
  XdmfInt32  Active;
};

#endif // __XdmfAttribute_h
