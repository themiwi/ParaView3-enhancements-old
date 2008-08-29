/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format */
/*                                                                 */
/*  Id : $Id: XdmfValuesHDF.cxx,v 1.11 2008-08-29 15:08:58 clarke Exp $  */
/*  Date : $Date: 2008-08-29 15:08:58 $ */
/*  Version : $Revision: 1.11 $ */
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
#include "XdmfObject.h"
#include "XdmfValuesHDF.h"
#include "XdmfHDF.h"
#include "XdmfDOM.h"
#include "XdmfDataStructure.h"
#include "XdmfArray.h"

XdmfValuesHDF::XdmfValuesHDF() {
    this->SetFormat(XDMF_FORMAT_HDF);
}

XdmfValuesHDF::~XdmfValuesHDF() {
}

XdmfArray *
XdmfValuesHDF::Read(XdmfArray *anArray){
    XdmfArray   *RetArray = anArray;
    XdmfString  DataSetName = 0;
    XdmfHDF     H5;

    if(!this->DataDesc){
        XdmfErrorMessage("DataDesc has not been set");
        return(NULL);
    }
    H5.SetWorkingDirectory(this->DOM->GetWorkingDirectory());
    XDMF_STRING_DUPLICATE(DataSetName, this->Get("CDATA"));
    if(!DataSetName || strlen(DataSetName) < 1){
        XdmfErrorMessage("Invalid HDF5 Dataset Name");
        return(NULL);
    }
    XDMF_WORD_TRIM(DataSetName);
    //! Possible Read from DSM. Make sure we're connected.
#ifndef XDMF_NO_MPI
    if(!this->DsmBuffer) this->SetDsmBuffer(anArray->GetDsmBuffer());
#endif
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
#ifndef XDMF_NO_MPI
    H5.SetDsmBuffer(this->DsmBuffer);
#endif
    if( H5.Open( DataSetName, "r" ) == XDMF_FAIL ) {
        XdmfErrorMessage("Can't Open Dataset " << DataSetName);
        if(!anArray) delete RetArray;
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
                if(RetArray->GetSelectionSize() < H5.GetSelectionSize()){
                    XdmfErrorMessage("Return Array No Large Enough to Hold Selected Data");
                    RetArray->SetShapeFromSelection(&H5);
                }
                // RetArray->SetShape(Rank, Count);
                // RetArray->SelectAll();
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
        XdmfDebug("Reading " << H5.GetSelectionSize() << " into Array of " << RetArray->GetSelectionSize() );
        if( H5.Read(RetArray) == NULL ){
            XdmfErrorMessage("Can't Read Dataset " << DataSetName);
            if(!anArray) delete RetArray;
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
XdmfValuesHDF::Write(XdmfArray *anArray, XdmfConstString aHeavyDataSetName){
    char* hds;
    XdmfHDF H5;

    if(!aHeavyDataSetName) aHeavyDataSetName = this->GetHeavyDataSetName();
    if(!aHeavyDataSetName){
        if(anArray->GetHeavyDataSetName()){
            aHeavyDataSetName = (XdmfConstString)anArray->GetHeavyDataSetName();
        }else{
            aHeavyDataSetName = this->GetUniqueName("Xdmf.h5:/Data");
        }
    }
    // Possible Write to DSM. Make sure we're connected
#ifndef XDMF_NO_MPI
    if(!this->DsmBuffer) this->SetDsmBuffer(anArray->GetDsmBuffer());
#endif
    XdmfDebug("Writing Values to " << aHeavyDataSetName);
    if(!this->DataDesc ){
        XdmfErrorMessage("DataDesc has not been set");
        return(XDMF_FAIL);
    }
    if(!anArray){
        XdmfErrorMessage("Array to Write is NULL");
        return(XDMF_FAIL);
    }
    XDMF_STRING_DUPLICATE(hds, aHeavyDataSetName);
    XDMF_WORD_TRIM( hds );
    this->Set("CDATA", hds);
    H5.CopyType(this->DataDesc);
    H5.CopyShape(this->DataDesc);
    H5.CopySelection(this->DataDesc);
#ifndef XDMF_NO_MPI
    H5.SetDsmBuffer(this->DsmBuffer);
#endif
    if(H5.Open(hds, "rw") == XDMF_FAIL){
        XdmfErrorMessage("Error Opening " << hds << " for Writing");
        delete [] hds ;
        return(XDMF_FAIL);
    }
    if(H5.Write(anArray) == XDMF_FAIL){
        XdmfErrorMessage("Error Writing " << hds );
        H5.Close();
        delete [] hds;
        return(XDMF_FAIL);
    }
    H5.Close();
    delete [] hds;
    return(XDMF_SUCCESS);
}
