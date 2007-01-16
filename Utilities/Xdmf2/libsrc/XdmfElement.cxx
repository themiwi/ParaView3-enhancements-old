/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id: XdmfElement.cxx,v 1.8 2007-01-16 22:01:37 clarke Exp $  */
/*  Date : $Date: 2007-01-16 22:01:37 $ */
/*  Version : $Revision: 1.8 $ */
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
#include "XdmfElement.h"
#include "XdmfDOM.h"
#include <libxml/tree.h>

XdmfElement::XdmfElement() {
    this->DOM = NULL;
    this->Element = NULL;
    this->IsReference = 0;
}

XdmfElement::~XdmfElement() {
}

XdmfInt32 XdmfElement::SetElement(XdmfXmlNode Element){
    if(!Element) {
        XdmfErrorMessage("Element is NULL");
        return(XDMF_FAIL);
    }
    // Clear the application data of the underlying node
    XdmfDebug("Clearing _private member of XML node");
    Element->_private = 0x0;
    this->Element = Element;
    return(XDMF_SUCCESS);
}

XdmfInt32 XdmfElement::InsertChildElement(XdmfXmlNode Child){
    if(!this->DOM) {
        XdmfErrorMessage("No DOM has been set");
        return(XDMF_FAIL);
    }
    if(!this->Element) {
        XdmfErrorMessage("Current Element is empty");
        return(XDMF_FAIL);
    }
    if(this->DOM->Insert(this->Element, Child)){
        return(XDMF_SUCCESS);
    }
    return(XDMF_FAIL);
}

XdmfInt32 XdmfElement::UpdateInformation(){
    if(!this->DOM) {
        XdmfErrorMessage("No DOM has been set");
        return(XDMF_FAIL);
    }
    if(!this->Element){
        XdmfErrorMessage("No XML Node has been set");
        return(XDMF_FAIL);
    }
    return(XDMF_SUCCESS);
}

XdmfInt32 XdmfElement::Update(){
    if(!this->DOM) {
        XdmfErrorMessage("No DOM has been set");
        return(XDMF_FAIL);
    }
    if(!this->Element){
        XdmfErrorMessage("No XML Node has been set");
        return(XDMF_FAIL);
    }
    XdmfDebug("Setting _private member of XML node");
    this->Element->_private = this;
    return(XDMF_SUCCESS);
}

XdmfConstString XdmfElement::Serialize(){
    if(!this->DOM) {
        XdmfErrorMessage("No DOM has been set");
        return(NULL);
    }
    if(!this->Element){
        XdmfErrorMessage("No XML Node has been set");
        return(NULL);
    }
    return(this->DOM->Serialize(this->Element));
}
XdmfConstString XdmfElement::GetElementType(){
    if(!this->DOM) {
        XdmfErrorMessage("No DOM has been set");
        return(NULL);
    }
    if(!this->Element){
        XdmfErrorMessage("No XML Node has been set");
        return(NULL);
    }
    return((XdmfConstString)this->Element->name);
}

XdmfInt32 XdmfElement::Build(){
    XdmfConstString  name;

    name = this->GetName();
    if(name && (strlen(name) > 1)){
        return(this->Set("Name", name));
    }
    return(XDMF_SUCCESS);
}

XdmfInt32 XdmfElement::Set(XdmfConstString Name, XdmfConstString Value){
    if(!this->DOM) {
        XdmfErrorMessage("No DOM has been set");
        return(XDMF_FAIL);
    }
    if(!this->Element){
        XdmfErrorMessage("No XML Node has been set");
        return(XDMF_FAIL);
    }
    this->DOM->Set(this->Element, Name, Value);
    return(XDMF_SUCCESS);
}


XdmfConstString XdmfElement::Get(XdmfConstString Name){
    if(!this->DOM) {
        XdmfErrorMessage("No DOM has been set");
        return(NULL);
    }
    if(!this->Element){
        XdmfErrorMessage("No XML Node has been set");
        return(NULL);
    }
    return(this->DOM->Get(this->Element, Name));
}
