/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Values              */
/*                                                                 */
/*  Id : $Id: XdmfDataStructure.cxx,v 1.14 2009-01-23 20:31:39 clarke Exp $  */
/*  Date : $Date: 2009-01-23 20:31:39 $ */
/*  Version : $Revision: 1.14 $ */
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
#include "XdmfDataStructure.h"
#include "XdmfArray.h"

XdmfDataStructure::XdmfDataStructure() {
    this->ItemType = XDMF_ITEM_UNIFORM;
}

XdmfDataStructure::~XdmfDataStructure() {
}

XdmfInt32 
XdmfDataStructure::UpdateInformation(){
    if(XdmfDataItem::UpdateInformation() != XDMF_SUCCESS) return(XDMF_FAIL);
    return(XDMF_SUCCESS);
}

