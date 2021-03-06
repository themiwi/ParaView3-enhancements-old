#include "vtkPSciVizDescriptiveStats.h"
#include "vtkSciVizStatisticsPrivate.h"

#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPDescriptiveStatistics.h"
#include "vtkTable.h"
#include "vtkVariantArray.h"

vtkStandardNewMacro(vtkPSciVizDescriptiveStats);
vtkCxxRevisionMacro(vtkPSciVizDescriptiveStats,"$Revision: 1.2 $");

vtkPSciVizDescriptiveStats::vtkPSciVizDescriptiveStats()
{
  this->SignedDeviations = 0;
}

vtkPSciVizDescriptiveStats::~vtkPSciVizDescriptiveStats()
{
}

void vtkPSciVizDescriptiveStats::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  os << indent << "SignedDeviations: " << this->SignedDeviations << "\n";
}

int vtkPSciVizDescriptiveStats::FitModel( vtkDataObject* modelDO, vtkTable* trainingData )
{
  // Get where we'll store the output statistical model.
  vtkTable* modelOut = vtkTable::SafeDownCast( modelDO );
  if ( ! modelOut )
    {
    vtkErrorMacro( "Output is not a table" );
    return 0;
    }

  // Create the statistics filter and run it
  vtkPDescriptiveStatistics* stats = vtkPDescriptiveStatistics::New();
  stats->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, trainingData );
  vtkIdType ncols = trainingData->GetNumberOfColumns();
  for ( vtkIdType i = 0; i < ncols; ++ i )
    {
    //stats->AddColumn( trainingData->GetColumnName( i ) );
    stats->SetColumnStatus( trainingData->GetColumnName( i ), 1 );
    }
  //stats->SetSignedDeviations( this->SignedDeviations ); // Shouldn't matter for model fitting, only affects assessed values.

  stats->SetLearnOption( true );
  stats->SetDeriveOption( true );
  stats->SetAssessOption( false );
  stats->Update();

  // Copy the output of the statistics filter to our output
  modelOut->ShallowCopy( stats->GetOutput( vtkStatisticsAlgorithm::OUTPUT_MODEL ) );
  stats->Delete();

  return 1;
}

int vtkPSciVizDescriptiveStats::AssessData( vtkTable* observations, vtkDataObject* assessedOut, vtkDataObject* modelOut )
{
  if ( ! assessedOut )
    {
    vtkErrorMacro( "No output data object." );
    return 0;
    }

  vtkFieldData* dataAttrOut = assessedOut->GetAttributesAsFieldData( this->AttributeMode );
  if ( ! dataAttrOut )
    {
    vtkErrorMacro( "No attributes of type " << this->AttributeMode << " on data object " << assessedOut );
    return 0;
    }

  // Shallow-copy the model so we don't create an infinite loop.
  vtkDataObject* modelCopy = modelOut->NewInstance();
  modelCopy->ShallowCopy( modelOut );

  // Create the statistics filter and run it
  vtkPDescriptiveStatistics* stats = vtkPDescriptiveStatistics::New();
  stats->SetInput( vtkStatisticsAlgorithm::INPUT_DATA, observations );
  stats->SetInput( vtkStatisticsAlgorithm::INPUT_MODEL, modelCopy );
  modelCopy->FastDelete();
  vtkIdType ncols = observations->GetNumberOfColumns();
  for ( vtkIdType i = 0; i < ncols; ++ i )
    {
    //stats->SetColumnStatus( observations->GetColumnName( i ), 1 );
    stats->AddColumn( observations->GetColumnName( i ) );
    }
  stats->SetSignedDeviations( this->SignedDeviations );

  stats->SetLearnOption( false );
  stats->SetDeriveOption( true );
  stats->SetAssessOption( true );
  stats->Update();

  vtkTable* assessTable = vtkTable::SafeDownCast( stats->GetOutput( vtkStatisticsAlgorithm::OUTPUT_DATA ) );
  vtkIdType ncolsout = assessTable ? assessTable->GetNumberOfColumns() : 0;
  for ( int i = ncols; i < ncolsout; ++ i )
    {
    dataAttrOut->AddArray( assessTable->GetColumn( i ) );
    }
  stats->Delete();

  return 1;
}

