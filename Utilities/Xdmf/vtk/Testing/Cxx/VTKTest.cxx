#include "vtkXdmfReader.h"
#include "vtkDataSet.h"
#include "vtkRectilinearGrid.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"

int main(int argc, char* argv[])
{
  if ( argc == 1 )
    {
    cout << "Usage: " << argv[0] << " <filename>" << endl;
    return 1;
    }
  vtkXdmfReader* vr = vtkXdmfReader::New();
  vr->SetFileName(argv[1]);
  //vr->UpdateInformation();

  //vtkDataArraySelection* ds = vr->GetCellDataArraySelection();
  //ds->DisableAllArrays();
  //ds->EnableArray(ds->GetArrayName(0));

//  vr->GetOutput()->Update();
  /*
  int *ue = vr->GetOutput()->GetUpdateExtent();
  cout << "Update extent: " 
    << ue[0] << " " << ue[1] << " " << ue[2] << " "
    << ue[3] << " " << ue[4] << " " << ue[5] << endl;
  vr->GetOutput()->SetUpdateExtent(75, 75, 0, 64, 0, 64);
  vr->GetOutput()->Update();
  ue = vr->GetOutput()->GetUpdateExtent();
  cout << "Update extent: " 
    << ue[0] << " " << ue[1] << " " << ue[2] << " "
    << ue[3] << " " << ue[4] << " " << ue[5] << endl;
  vr->GetOutput()->SetUpdateExtent(0, -1, 0, -1, 0, -1);
  vr->GetOutput()->Update();
  ue = vr->GetOutput()->GetUpdateExtent();
  cout << "Update extent: " 
    << ue[0] << " " << ue[1] << " " << ue[2] << " "
    << ue[3] << " " << ue[4] << " " << ue[5] << endl;
    */
  vr->GetOutput()->SetUpdateExtent(75, 75, 0, 64, 0, 64);
  vr->GetOutput()->Update();
/*
  cout << "------------- UpdateInformation --------" << endl;
  vr->UpdateInformation();

  if ( argc > 2 )
    {
    vtkDataArraySelection* ds = vr->GetCellDataArraySelection();
    ds->DisableAllArrays();
    ds->EnableArray(ds->GetArrayName(0));
    vr->SetStride(2, 3, 4);
    cout << "------------- Update -------------------" << endl;
    vr->Update();
    cout << "------------- Reader: ------------------" << endl;
    //vr->Print(cout);
    cout << "------------- Output: ------------------" << endl;
    cout << "Number of cells: " << vr->GetOutput()->GetNumberOfCells() << endl;
    //vr->GetOutput()->Print(cout);
    vtkRectilinearGrid* rg = vtkRectilinearGrid::SafeDownCast( vr->GetOutput() );
    if ( rg )
      {
      vtkDataArray* array = rg->GetCellData()->GetArray(0);
      //array->Print(cout);
      cout << "Number of tuples of array: " << array->GetNumberOfTuples() << endl;
      cout << "Number of X coords: " << rg->GetXCoordinates()->GetNumberOfTuples() << endl;
      cout << "Number of y coords: " << rg->GetYCoordinates()->GetNumberOfTuples() << endl;
      cout << "Number of z coords: " << rg->GetZCoordinates()->GetNumberOfTuples() << endl;


      }
    cout << "------------- Delete  ------------------" << endl;
    }
  else
    {
    vr->Print(cout);
    }
    */
  vr->Delete();
  return 0;
}
