/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id: XdmfDataStructure.cxx,v 1.5 2007-01-04 21:54:05 clarke Exp $  */
/*  Date : $Date: 2007-01-04 21:54:05 $ */
/*  Version : $Revision: 1.5 $ */
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
#include "XdmfDataDesc.h"
#include "XdmfArray.h"

// Supported Xdmf Formats
#include "XdmfValuesXML.h"
#include "XdmfValuesHDF.h"

XdmfDataStructure::XdmfDataStructure() {
    this->Values = NULL;
    this->DataDesc = new XdmfDataDesc;
    this->DataDescIsMine = 1;
    this->Array = new XdmfArray;
    this->ArrayIsMine = 1;
    this->Array->SetNumberType(XDMF_FLOAT32_TYPE);
    this->Array->SetNumberOfElements(3);
    this->Format = XDMF_FORMAT_XML;
    this->HeavyDataSetName = NULL;
}

XdmfDataStructure::~XdmfDataStructure() {
    if(this->Array && this->ArrayIsMine ) delete this->Array;
    if(this->DataDesc && this->DataDescIsMine) delete this->DataDesc;
    if(this->Values) delete this->Values;
}

XdmfInt32 
XdmfDataStructure::SetArray(XdmfArray *Array){
    if(this->Array && this->ArrayIsMine) delete this->Array;
    this->ArrayIsMine = 0;
    this->Array = Array;
    return(XDMF_SUCCESS);
}

XdmfInt32 
XdmfDataStructure::SetDataDesc(XdmfDataDesc *DataDesc){
    if(this->DataDesc && this->DataDescIsMine) delete this->DataDesc;
    this->DataDescIsMine = 0;
    this->DataDesc = DataDesc;
    return(XDMF_SUCCESS);
}

XdmfInt32 XdmfDataStructure::UpdateInformation(){
    XdmfConstString Value;
    XdmfInt32   Precision = 4;

    if(XdmfElement::UpdateInformation() != XDMF_SUCCESS) return(XDMF_FAIL);
    Value = this->Get("Name");
    if(Value) this->SetName(Value);
    Value = this->Get("Dimensions");
    if(!Value) {
        XdmfErrorMessage("Dimensions are not set in XML Element");
        return(XDMF_FAIL);
    }
    if(!this->DataDesc) this->DataDesc = new XdmfDataDesc;
    this->DataDesc->SetShapeFromString(Value);
    Value = this->Get("Precision");
    if(Value) Precision = atoi(Value);
    Value = this->Get("Type");
    // Only allow Simple for now.
    if(XDMF_WORD_CMP(Value, "Char")){
        this->DataDesc->SetNumberType(XDMF_INT8_TYPE);
    } else if(XDMF_WORD_CMP(Value, "UChar")){
        this->DataDesc->SetNumberType(XDMF_UINT8_TYPE);
    } else if(XDMF_WORD_CMP(Value, "Int")){
        if(Precision == 8){
            this->DataDesc->SetNumberType(XDMF_INT64_TYPE);
        }else{
            this->DataDesc->SetNumberType(XDMF_INT32_TYPE);
        }
    } else {
        if(Precision == 8){
            this->DataDesc->SetNumberType(XDMF_FLOAT64_TYPE);
        }else{
            this->DataDesc->SetNumberType(XDMF_FLOAT32_TYPE);
        }
    }
    Value = this->Get("Format");
    // Currently XML or HDF5
    if(XDMF_WORD_CMP(Value, "HDF")){
        this->SetFormat(XDMF_FORMAT_HDF);
    } else if(XDMF_WORD_CMP(Value, "HDF5")){
        this->SetFormat(XDMF_FORMAT_HDF);
    } else if(XDMF_WORD_CMP(Value, "H5")){
        this->SetFormat(XDMF_FORMAT_HDF);
    } else if(XDMF_WORD_CMP(Value, "XML")){
        this->SetFormat(XDMF_FORMAT_XML);
    }else if(Value){
        XdmfErrorMessage("Unsupported DataStructure Format :" << Value);
        return(XDMF_FAIL);
    }
    return(XDMF_SUCCESS);
}

XdmfInt32 XdmfDataStructure::Update(){
    if(XdmfElement::Update() != XDMF_SUCCESS) return(XDMF_FAIL);
    if(this->Array->CopyType(this->DataDesc) != XDMF_SUCCESS) return(XDMF_FAIL);
    if(this->Array->CopyShape(this->DataDesc) != XDMF_SUCCESS) return(XDMF_FAIL);
    if(this->Array->CopySelection(this->DataDesc) != XDMF_SUCCESS) return(XDMF_FAIL);
    if(this->CheckValues(this->Format) != XDMF_SUCCESS){
        XdmfErrorMessage("Error Accessing Internal XdmfValues");
        return(XDMF_FAIL);
    }
    switch (this->Format) {
        case XDMF_FORMAT_HDF :
            if(!((XdmfValuesHDF *)Values)->Read(this->Array)){
                XdmfErrorMessage("Reading Values Failed");
                return(XDMF_FAIL);
            }
            this->SetHeavyDataSetName(Values->GetHeavyDataSetName());
            break;
        case XDMF_FORMAT_XML :
            if(!((XdmfValuesXML *)Values)->Read(this->Array)){
                XdmfErrorMessage("Reading Values Failed");
                return(XDMF_FAIL);
            }
            break;
        default :
            XdmfErrorMessage("Unsupported Data Format");
            return(XDMF_FAIL);
    }
    return(XDMF_SUCCESS);
}

XdmfString XdmfDataStructure::GetDataValues(XdmfInt64 Index, XdmfInt64 NumberOfValues, XdmfInt64 ArrayStride){
    if(!this->Array) return(NULL);
    return(this->Array->GetValues(Index, NumberOfValues, ArrayStride));
}

XdmfInt32 XdmfDataStructure::SetDataValues(XdmfInt64 Index, XdmfConstString Values, XdmfInt64 ArrayStride, XdmfInt64 ValuesStride){
    if(!this->Array){
        XdmfErrorMessage("DataStructure has no XdmfArray");
        return(XDMF_FAIL);
    }
    return(this->Array->SetValues(Index, Values, ArrayStride, ValuesStride));
}

XdmfInt32   XdmfDataStructure::GetRank() {
    if(!this->DataDesc){
        XdmfErrorMessage("There is no XdmfDataDesc");
        return(XDMF_FAIL);
    }
    return(this->DataDesc->GetRank());
}

XdmfInt32 XdmfDataStructure::SetShape(XdmfInt32 Rank, XdmfInt64 *Dimensions){
    if(!this->DataDesc){
        XdmfErrorMessage("There is no XdmfDataDesc");
        return(XDMF_FAIL);
    }
    return(this->DataDesc->SetShape(Rank, Dimensions));
}

XdmfInt32 XdmfDataStructure::GetShape(XdmfInt64 *Dimensions){
    if(!this->DataDesc){
        XdmfErrorMessage("There is no XdmfDataDesc");
        return(XDMF_FAIL);
    }
    return(this->DataDesc->GetShape(Dimensions));
}

XdmfInt32 XdmfDataStructure::SetDimensionsFromString(XdmfConstString Dimensions){
    if(!this->DataDesc){
        XdmfErrorMessage("There is no XdmfDataDesc");
        return(XDMF_FAIL);
    }
    return(this->DataDesc->SetShapeFromString(Dimensions));
}

XdmfConstString XdmfDataStructure::GetShapeAsString(){
    if(!this->DataDesc){
        XdmfErrorMessage("There is no XdmfDataDesc");
        return(NULL);
    }
    return(this->DataDesc->GetShapeAsString());
}

XdmfInt32 XdmfDataStructure::Build(){
    XdmfDataDesc *DataDesc = this->DataDesc;
    if(XdmfElement::Build() != XDMF_SUCCESS) return(XDMF_FAIL);
    if(this->Array) DataDesc = this->Array;
    this->Set("Dimensions", DataDesc->GetShapeAsString());
    this->Set("Type", XdmfTypeToClassString(DataDesc->GetNumberType()));
    switch (DataDesc->GetElementSize()) {
        case 8 :
            this->Set("Precision", "8");
            break;
        case 4 :
            this->Set("Precision", "4");
            break;
        case 1 :
            this->Set("Precision", "1");
            break;
        default :
            break;
    }
    if(this->CheckValues(this->Format) != XDMF_SUCCESS){
        XdmfErrorMessage("Error Accessing Internal XdmfValues");
        return(XDMF_FAIL);
    }
    this->Values->SetDataDesc(DataDesc);
    switch (this->Format) {
        case XDMF_FORMAT_HDF :
            XdmfDebug("Writing Values in HDF Format");
            Values->SetHeavyDataSetName(this->GetHeavyDataSetName());
            if(((XdmfValuesHDF *)Values)->Write(this->Array) != XDMF_SUCCESS){
                XdmfErrorMessage("Writing Values Failed");
                return(XDMF_FAIL);
            }
            this->Set("Format", "HDF");
            break;
        case XDMF_FORMAT_XML :
            XdmfDebug("Writing Values in XML Format");
            if(((XdmfValuesXML *)Values)->Write(this->Array) != XDMF_SUCCESS){
                XdmfErrorMessage("Writing Values Failed");
                return(XDMF_FAIL);
            }
            this->Set("Format", "XML");
            break;
        default :
            XdmfErrorMessage("Unsupported Data Format");
            return(XDMF_FAIL);
    }
    return(XDMF_SUCCESS);
}

XdmfInt32
XdmfDataStructure::CheckValues(XdmfInt32 Format){
    if(this->Values){
        // Exists
        if(this->Values->Format != Format){
            // Wrong Format
            XdmfDebug("CheckValues Changing Format");
            delete this->Values;
            this->Values = NULL;
        }
    }
    if(!this->Values){
        // Create One of the Proper Format
        switch (this->Format) {
            case XDMF_FORMAT_HDF :
                this->Values = (XdmfValues *)new XdmfValuesHDF();
                break;
            case XDMF_FORMAT_XML :
                this->Values = (XdmfValues *)new XdmfValuesXML();
                break;
            default :
                XdmfErrorMessage("Unsupported Data Format");
                return(XDMF_FAIL);
        }
    }
    if(!this->Values){
        XdmfErrorMessage("Error Creating new XdmfValues");
        return(XDMF_FAIL);
    }
    if(this->Values->Inherit(this) != XDMF_SUCCESS){
        XdmfErrorMessage("Error Inheriting DOM, Element, and DataDesc");
        return(XDMF_FAIL);
    }
    return(XDMF_SUCCESS);
}


