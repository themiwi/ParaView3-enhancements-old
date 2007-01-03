/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and ValuesXML              */
/*                                                                 */
/*  Id : $Id: XdmfValuesXML.cxx,v 1.4 2007-01-03 21:43:09 clarke Exp $  */
/*  Date : $Date: 2007-01-03 21:43:09 $ */
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
#include "XdmfValuesXML.h"
#include "XdmfDataStructure.h"
#include "XdmfArray.h"

XdmfValuesXML::XdmfValuesXML() {
    this->SetFormat(XDMF_FORMAT_XML);
}

XdmfValuesXML::~XdmfValuesXML() {
}

XdmfArray *
XdmfValuesXML::Read(XdmfArray *Array){
    XdmfArray   *RetArray = Array;

    if(!this->DataDesc){
        XdmfErrorMessage("DataDesc has not been set");
        return(NULL);
    }
    // Allocate Array if Necessary
    if(!RetArray){
        RetArray = new XdmfArray();
        RetArray->CopyType(this->DataDesc);
        RetArray->CopyShape(this->DataDesc);
        RetArray->CopySelection(this->DataDesc);
    }
    if(RetArray->SetValues(0, this->Get("CDATA")) != XDMF_SUCCESS){
        XdmfErrorMessage("Error Accessing Actual Data Values");
        if(!Array) delete RetArray;
        RetArray = NULL;
    }
    return(RetArray);
}

XdmfInt32
XdmfValuesXML::Write(XdmfArray *Array, XdmfConstString HeavyDataSetName){

    XdmfConstString DataValues;
    ostrstream   StringOutput;
    XdmfInt32   rank, r;
    XdmfInt64   i, index, nelements, len, idims[XDMF_MAX_DIMENSION], dims[XDMF_MAX_DIMENSION];

    if(!this->DataDesc ){
        XdmfErrorMessage("DataDesc has not been set");
        return(XDMF_FAIL);
    }
    if(!Array){
        XdmfErrorMessage("Array to Write is NULL");
        return(XDMF_FAIL);
    }
    rank = this->DataDesc->GetShape(dims);
    for(i = 0 ; i < rank ; i++){
        idims[i] = dims[i];
    }
    // At most 10 values per line
    len = MIN(dims[rank - 1], 10);
    nelements = this->DataDesc->GetNumberOfElements();
    index = 0;
    StringOutput << endl;
    while(nelements){
        r = rank - 1;
        len = MIN(len, nelements);
        DataValues = Array->GetValues(index, len);
        StringOutput << DataValues << endl;
        index += len;
        nelements -= len;
        dims[r] -= len;
        // End of Smallest dimension ?
        if(nelements && r && (dims[r] <= 0)){
            // Reset
            dims[r] = idims[r];
            // Go Backwards thru dimensions
            while(r){
                r--;
                dims[r]--;
                // Is dim now 0
                if(dims[r] <= 0){
                    // Add an Endl and keep going
                    StringOutput << endl;
                    dims[r] = idims[r];
                }else{
                    // Still some left
                    break;
                }
            }
        }
    }
    return(this->Set("CDATA", StringOutput.str()));
}
