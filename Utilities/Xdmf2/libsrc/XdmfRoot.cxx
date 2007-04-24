/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id: XdmfRoot.cxx,v 1.1 2007-04-24 14:21:49 clarke Exp $  */
/*  Date : $Date: 2007-04-24 14:21:49 $ */
/*  Version : $Revision: 1.1 $ */
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

XdmfRoot::XdmfRoot() {
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

XdmfInt32 XdmfRoot::Build(){
    static char VersionBuf[80];
    ostrstream  Version(VersionBuf,80);

    if(XdmfElement::Build() != XDMF_SUCCESS) return(XDMF_FAIL);
    // Version and XInclude
    Version << this->Version << ends;
    this->Set("Version", (XdmfConstString)Version.str());
    return(XDMF_SUCCESS);
}
