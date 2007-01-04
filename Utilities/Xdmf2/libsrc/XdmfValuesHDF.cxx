/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format */
/*                                                                 */
/*  Id : $Id: XdmfValuesHDF.cxx,v 1.2 2007-01-04 21:54:05 clarke Exp $  */
/*  Date : $Date: 2007-01-04 21:54:05 $ */
/*  Version : $Revision: 1.2 $ */
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
#include "XdmfValuesHDF.h"
#include "XdmfHDF.h"
#include "XdmfDataStructure.h"
#include "XdmfArray.h"

XdmfValuesHDF::XdmfValuesHDF() {
    this->SetFormat(XDMF_FORMAT_HDF);
}

XdmfValuesHDF::~XdmfValuesHDF() {
}

XdmfArray *
XdmfValuesHDF::Read(XdmfArray *Array){
    XdmfArray   *RetArray = Array;
    XdmfString  DataSetName = 0;
    XdmfHDF     H5;

    if(!this->DataDesc){
        XdmfErrorMessage("DataDesc has not been set");
        return(NULL);
    }
    H5.SetWorkingDirectory(this->GetWorkingDirectory());
    XDMF_STRING_DUPLICATE(DataSetName, this->Get("CDATA"));
    if(!DataSetName || strlen(DataSetName) < 1){
        XdmfErrorMessage("Invalid HDF5 Dataset Name");
        return(NULL);
    }
    XDMF_WORD_TRIM(DataSetName);
    XdmfDebug("Opening HDF5 Data for Reading : " << DataSetName);
    // Allocate Array if Necessary
    if(!RetArray){
        RetArray = new XdmfArray();
        if(!RetArray){
            XdmfErrorMessage("Error Allocating New Array");
            return(NULL);
        }
        RetArray->CopyType(this->DataDesc);
        RetArray->CopyShape(this->DataDesc);
        RetArray->CopySelection(this->DataDesc);
        RetArray->Allocate();
    }
    if( H5.Open( DataSetName, "r" ) == XDMF_FAIL ) {
        XdmfErrorMessage("Can't Open Dataset " << DataSetName);
        if(!Array) delete RetArray;
        RetArray = NULL;
    }else{
        if(this->DataDesc->GetSelectionSize() != H5.GetNumberOfElements() ){
          // We're not reading the entire dataset
            if( this->DataDesc->GetSelectionType() == XDMF_HYPERSLAB ){
                XdmfInt32  Rank;
                XdmfInt64  Start[ XDMF_MAX_DIMENSION ];
                XdmfInt64  Stride[ XDMF_MAX_DIMENSION ];
                XdmfInt64  Count[ XDMF_MAX_DIMENSION ];
        
                // Select the HyperSlab from HDF5
                Rank = this->DataDesc->GetHyperSlab( Start, Stride, Count );
                H5.SelectHyperSlab( Start, Stride, Count );
                RetArray->SetShape(Rank, Count);
                RetArray->SelectAll();
            } else {
                XdmfInt64  NumberOfCoordinates;
                XdmfInt64  *Coordinates;


                // Select Parametric Coordinates from HDF5
                NumberOfCoordinates = this->DataDesc->GetSelectionSize();
                Coordinates = this->DataDesc->GetCoordinates();
                RetArray->SetNumberOfElements(NumberOfCoordinates);
                H5.SelectCoordinates(NumberOfCoordinates, Coordinates);
                delete Coordinates;
                }
            }

        if( H5.Read(RetArray) == NULL ){
            XdmfErrorMessage("Can't Read Dataset " << DataSetName);
            if(!Array) delete RetArray;
            RetArray = NULL;
        }else{
            this->SetHeavyDataSetName(DataSetName);
        }
    H5.Close();
    }
    delete [] DataSetName;
    return(RetArray);
}

XdmfInt32
XdmfValuesHDF::Write(XdmfArray *Array, XdmfConstString HeavyDataSetName){
    char* heavyDataset;
    XdmfHDF H5;

    if(!HeavyDataSetName) HeavyDataSetName = this->GetHeavyDataSetName();
    if(!HeavyDataSetName){
        HeavyDataSetName = "Xdmf.h5:/Data";
    }
    XdmfDebug("Writing Values to " << HeavyDataSetName);
    if(!this->DataDesc ){
        XdmfErrorMessage("DataDesc has not been set");
        return(XDMF_FAIL);
    }
    if(!Array){
        XdmfErrorMessage("Array to Write is NULL");
        return(XDMF_FAIL);
    }
    heavyDataset = new char [ strlen(HeavyDataSetName) + 1 ];
    strcpy(heavyDataset, HeavyDataSetName);
    XDMF_WORD_TRIM( heavyDataset );
    this->Set("CDATA", heavyDataset);
    H5.CopyType(this->DataDesc);
    H5.CopyShape(this->DataDesc);
    H5.CopySelection(this->DataDesc);
    if(H5.Open(heavyDataset, "rw") == XDMF_FAIL){
        XdmfErrorMessage("Error Opening " << heavyDataset << " for Writing");
        delete [] heavyDataset;
        return(XDMF_FAIL);
    }
    if(H5.Write(Array) == XDMF_FAIL){
        XdmfErrorMessage("Error Writing " << heavyDataset);
        H5.Close();
        delete [] heavyDataset;
        return(XDMF_FAIL);
    }
    H5.Close();
    delete [] heavyDataset;
    return(XDMF_SUCCESS);
}
