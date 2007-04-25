/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id: XdmfRoot.cxx,v 1.3 2007-04-25 16:23:29 clarke Exp $  */
/*  Date : $Date: 2007-04-25 16:23:29 $ */
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
#include "XdmfRoot.h"
#include "XdmfDOM.h"

XdmfRoot::XdmfRoot() {
    this->SetElementName("Xdmf");
    this->Version = XDMF_VERSION;
    this->XInclude = 1;
}

XdmfRoot::~XdmfRoot() {
}

XdmfInt32 XdmfRoot::UpdateInformation(){
    XdmfConstString Value;

    XdmfElement::UpdateInformation();
    Value = this->Get("Version");
    if(Value) this->SetVersion(atof(Value));
    Value = this->Get("XInclude");
    if(!Value) this->SetXInclude(atoi(Value));
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfRoot::Insert( XdmfElement *Child){
    if(Child && (
        XDMF_WORD_CMP(Child->GetElementName(), "Domain") ||
        XDMF_WORD_CMP(Child->GetElementName(), "DataItem") ||
        XDMF_WORD_CMP(Child->GetElementName(), "Information")
        )){
        return(XdmfElement::Insert(Child));
    }else{
        XdmfErrorMessage("Xdmf Root can only Insert Domain | DataItem | Information elements, not a " << Child->GetElementName());
    }
    return(XDMF_FAIL);
}

XdmfInt32 XdmfRoot::Build(){
    static char VersionBuf[80];
    ostrstream  Version(VersionBuf,80);

    if(!this->GetElement()){
        if(this->GetDOM()){
            XdmfXmlNode  node;

            node = this->GetDOM()->Create();
            this->SetElement(node);
        }
    }
    if(XdmfElement::Build() != XDMF_SUCCESS) return(XDMF_FAIL);
    // Version and XInclude
    Version << this->Version << ends;
    this->Set("Version", (XdmfConstString)Version.str());
    return(XDMF_SUCCESS);
}
