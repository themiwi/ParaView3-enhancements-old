/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id: XdmfFormatHDF.h,v 1.6 2004-01-15 21:43:56 andy Exp $  */
/*  Date : $Date: 2004-01-15 21:43:56 $ */
/*  Version : $Revision: 1.6 $ */
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
#ifndef __XdmfFormatHDF_h
#define __XdmfFormatHDF_h


#include "XdmfFormat.h"

class XdmfArray;
class XdmfDataDesc;
class XdmfXNode;

//! Class for handeling XML describing HDF5 files
/*!
	An example tag for XML describing HDF5
\verbatim
<DataStructure
  Name="XXX"
  Rank="2"
  Dimensions="2 4"
  Precision="4"
  DataType="Float"
  Format="HDF">
  MyFile.h5:/Values
</DataStructure>
\endverbatim

*/
class XDMF_EXPORT XdmfFormatHDF : public XdmfFormat {

public :

  XdmfFormatHDF();
  ~XdmfFormatHDF();

  XdmfConstString GetClassName() { return("XdmfFormatHDF"); } ;

//! Return an Array from the DOM Element
  XdmfArray  *ElementToArray( XdmfXNode *Element,
            XdmfDataDesc  *Desc = NULL,
            XdmfArray *Array = NULL );
//! Write an Array. The Shape of the Array is overridden by the Optional Desc.
  XdmfXNode    *ArrayToElement( XdmfArray *Array,
        XdmfConstString HeavyDataset = NULL,
        XdmfXNode *Element = NULL,
        XdmfDataDesc *Desc = NULL );

};

#endif
