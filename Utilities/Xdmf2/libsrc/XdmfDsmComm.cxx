/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id: XdmfDsmComm.cxx,v 1.1 2007-05-08 16:31:53 clarke Exp $  */
/*  Date : $Date: 2007-05-08 16:31:53 $ */
/*  Version : $Revision: 1.1 $ */
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
#include "XdmfDsmComm.h"


XdmfDsmComm::XdmfDsmComm() {
}

XdmfDsmComm::~XdmfDsmComm() {
}

XdmfInt32
XdmfDsmComm::Init(){
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDsmComm::Check(XdmfDsmMsg *Msg){
    if(Msg->Tag <= 0) Msg->Tag = XDMF_DSM_DEFAULT_TAG;
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDsmComm::Receive(XdmfDsmMsg *Msg){
    if(Msg->Tag <= 0) Msg->Tag = XDMF_DSM_DEFAULT_TAG;
    if(Msg->Size <= 0 ){
        XdmfErrorMessage("Cannot Receive Message of Size = " << Msg->Size);
        return(XDMF_FAIL);
    }
    if(Msg->Data <= 0 ){
        XdmfErrorMessage("Cannot Receive Message into Data Buffer = " << Msg->Size);
        return(XDMF_FAIL);
    }
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDsmComm::Send(XdmfDsmMsg *Msg){
    if(Msg->Tag <= 0) Msg->Tag = XDMF_DSM_DEFAULT_TAG;
    if(Msg->Size <= 0 ){
        XdmfErrorMessage("Cannot Send Message of Size = " << Msg->Size);
        return(XDMF_FAIL);
    }
    if(Msg->Data <= 0 ){
        XdmfErrorMessage("Cannot Send Message from Data Buffer = " << Msg->Size);
        return(XDMF_FAIL);
    }
    return(XDMF_SUCCESS);
}
