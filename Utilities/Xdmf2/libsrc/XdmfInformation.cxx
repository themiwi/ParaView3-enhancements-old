/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id: XdmfInformation.cxx,v 1.3 2006-12-14 22:15:14 clarke Exp $  */
/*  Date : $Date: 2006-12-14 22:15:14 $ */
/*  Version : $Revision: 1.3 $ */
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
#include "XdmfInformation.h"
#include "XdmfDOM.h"

XdmfInformation::XdmfInformation() {
    this->Value = NULL;
}

XdmfInformation::~XdmfInformation() {
}

XdmfInt32 XdmfInformation::UpdateInformation(){
    XdmfConstString Value;

    XdmfElement::UpdateInformation();
    Value = this->Get("Name");
    if(Value) this->SetName(Value);
    Value = this->Get("Value");
    if(!Value) Value = this->Get("CDATA");
    if(Value) this->SetValue(Value);
    return(XDMF_SUCCESS);
}

XdmfInt32 XdmfInformation::UpdateDOM(){
    if(XdmfElement::UpdateDOM() != XDMF_SUCCESS) return(XDMF_FAIL);
    // If Value isn't already an XML Attribute and
    // the value is > 10 chars, put it in the CDATA
    if((this->Get("Value") == NULL)  && (strlen(this->Value) > 10)){
        this->Set("CDATA", this->Value);
    }else{
        this->Set("Value", this->Value);
    }
    return(XDMF_SUCCESS);
}
