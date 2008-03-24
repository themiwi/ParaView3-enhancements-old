/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPythonProgrammableFilter.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPythonProgrammableFilter.h"

#include "vtkDataObject.h"
#include "vtkDataObjectTypes.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkOnePieceExtentTranslator.h"
#include "vtkPVOptions.h"
#include "vtkPVPythonInterpretor.h"
#include "vtkProcessModule.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtkstd/map>
#include <vtkstd/string>

vtkCxxRevisionMacro(vtkPythonProgrammableFilter, "$Revision: 1.27 $");
vtkStandardNewMacro(vtkPythonProgrammableFilter);

//----------------------------------------------------------------------------

typedef vtkstd::map<vtkstd::string, vtkstd::string> ParametersT;

class vtkPythonProgrammableFilterImplementation
{
public:
  vtkPythonProgrammableFilterImplementation() :
    Interpretor(NULL), PythonVarDefined(false)
  {
  }

  void DestroyInterpretor()
    {
      if (this->Interpretor)
        {
        this->PythonVarDefined = false;
        // The following is necessary because the Delete() may
        // cause the destruction of vtkPythonProgrammableFilter
        // which calls DestroyInterpretor() in its destructor.
        // If this->Interpretor is not set to 0, it will be
        // deleted a second time causing segmentation fault.
        vtkPVPythonInterpretor* interp = this->Interpretor;
        this->Interpretor = 0;
        interp->Delete();
        }
    }

  vtkPVPythonInterpretor* Interpretor;
  
  // Stores name-value parameters that will be passed to running scripts
  ParametersT Parameters;

  // PythonVarDefined is true if python interp has a variable
  // that points to this filter. This means thatthere is reference 
  // loop due to:
  // filter <-> interpretor
  bool PythonVarDefined;
};

//----------------------------------------------------------------------------
vtkPythonProgrammableFilter::vtkPythonProgrammableFilter() :
  Implementation(new vtkPythonProgrammableFilterImplementation())
{
  this->Script = NULL;
  this->InformationScript = NULL;
  this->SetExecuteMethod(vtkPythonProgrammableFilter::ExecuteScript, this);
  this->OutputDataSetType = VTK_POLY_DATA;
}

//----------------------------------------------------------------------------
vtkPythonProgrammableFilter::~vtkPythonProgrammableFilter()
{
  this->SetScript(NULL);
  this->SetInformationScript(NULL);

  this->Implementation->DestroyInterpretor();
  delete this->Implementation;
}

//----------------------------------------------------------------------------
void vtkPythonProgrammableFilter::UnRegister(vtkObjectBase *o)
{
  this->Superclass::UnRegister(o);
  bool hasRefLoop = this->Implementation->PythonVarDefined ? true : false;
  if (this->GetReferenceCount() == 3 && hasRefLoop)
    {
    this->Implementation->DestroyInterpretor();
    }
}

//----------------------------------------------------------------------------
int vtkPythonProgrammableFilter::RequestDataObject(
  vtkInformation* vtkNotUsed(request), 
  vtkInformationVector** inputVector , 
  vtkInformationVector* outputVector)
{
  if (this->OutputDataSetType == VTK_DATA_SET)
    {
    // Output type is same as input
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    if (!inInfo)
      {
      return 0;
      }
    vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
    if (input)
      {
      // for each output
      for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
        {
        vtkInformation* info = outputVector->GetInformationObject(i);
        vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());
        
        if (!output || !output->IsA(input->GetClassName())) 
          {
          vtkDataObject* newOutput = input->NewInstance();
          newOutput->SetPipelineInformation(info);
          newOutput->Delete();
          this->GetOutputPortInformation(0)->Set(
            vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
          }
        }
      return 1;
      }
    return 0;
    }

  const char *outTypeStr = 
    vtkDataObjectTypes::GetClassNameFromTypeId(this->OutputDataSetType);

  // for each output
  for(int i=0; i < this->GetNumberOfOutputPorts(); ++i)
    {
    vtkInformation* info = outputVector->GetInformationObject(i);
    vtkDataObject *output = info->Get(vtkDataObject::DATA_OBJECT());
    if (!output || !output->IsA(outTypeStr)) 
      {
      vtkDataObject* newOutput = 
        vtkDataObjectTypes::NewDataObject(this->OutputDataSetType);
      if (!newOutput)
        {
        vtkErrorMacro("Could not create chosen output data type: "
                      << outTypeStr);
        return 0;
        }
      newOutput->SetPipelineInformation(info);
      this->GetOutputPortInformation(0)->Set(
        vtkDataObject::DATA_EXTENT_TYPE(), newOutput->GetExtentType());
      newOutput->Delete();
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPythonProgrammableFilter::RequestInformation(
  vtkInformation*, 
  vtkInformationVector**, 
  vtkInformationVector* outputVector)
{
  // Start with a new interpretor
  this->Implementation->DestroyInterpretor();

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Setup ExtentTranslator so that all downstream piece requests are
  // converted to whole extent update requests, as need by the histogram filter.
  vtkStreamingDemandDrivenPipeline* sddp = 
    vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
  if (strcmp(
      sddp->GetExtentTranslator(outInfo)->GetClassName(), 
      "vtkOnePieceExtentTranslator") != 0)
    {
    vtkExtentTranslator* et = vtkOnePieceExtentTranslator::New();
    sddp->SetExtentTranslator(outInfo, et);
    et->Delete();
    }

  if (this->InformationScript)
    {
    this->Exec(this->InformationScript, "RequestInformation");
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPythonProgrammableFilter::SetParameter(const char *raw_name, 
                                               const char *raw_value)
{
  const vtkstd::string name = raw_name ? raw_name : "";
  const vtkstd::string value = raw_value ? raw_value : "";

  if(name.empty())
    {
    vtkErrorMacro(<< "cannot set parameter with empty name");
    return;
    }
    
  this->Implementation->Parameters[name] = value;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPythonProgrammableFilter::ClearParameters()
{
  this->Implementation->Parameters.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPythonProgrammableFilter::ExecuteScript(void *arg)
{  
  vtkPythonProgrammableFilter *self = 
    static_cast<vtkPythonProgrammableFilter*>(arg);
  if (self)
    {
    self->Exec(self->GetScript(), "RequestData");
    }
}

//----------------------------------------------------------------------------
void vtkPythonProgrammableFilter::Exec(const char* script,
                                       const char* funcname)
{
  if (!script || !strlen(script))
    {
    return;
    }

  if (this->Implementation->Interpretor == NULL)
    {
    this->Implementation->Interpretor = vtkPVPythonInterpretor::New();
    this->Implementation->Interpretor->SetCaptureStreams(true);
    const char* argv0 = vtkProcessModule::GetProcessModule()->
      GetOptions()->GetArgv0();
    this->Implementation->Interpretor->InitializeSubInterpretor(
      1, (char**)&argv0);
    }

  // Construct a script that defines a function
  vtkstd::string fscript;
  fscript  = "def ";
  fscript += funcname;

  // Set the parameters defined by user.
  fscript += "(self):\n";
  for(ParametersT::const_iterator parameter = 
        this->Implementation->Parameters.begin();
      parameter != this->Implementation->Parameters.end();
      ++parameter)
    {
    fscript += "\t" + parameter->first + " = " + parameter->second + "\n";
    } 
  
  // Indent user script
  fscript += "\t";
  vtkstd::string orgscript(script);
  // Remove DOS line endings. They confuse the indentation code below.
  orgscript.erase(
    vtkstd::remove(orgscript.begin(), orgscript.end(), '\r'), orgscript.end());

  vtkstd::string::iterator it = orgscript.begin();
  for(; it != orgscript.end(); it++)
    {
    fscript += *it;
    // indent new lines
    if (*it == '\n')
      {
      fscript += "\t";
      }
    }
  fscript += "\n";
  this->Implementation->Interpretor->RunSimpleString(fscript.c_str());

  if (!this->Implementation->PythonVarDefined)
    {
    vtkstd::string initscript;
    initscript = "from paraview import vtk\n";

    // Set self to point to this
    char addrofthis[1024];
    sprintf(addrofthis, "%p", this);    
    char *aplus = addrofthis; 
    if ((addrofthis[0] == '0') && 
        ((addrofthis[1] == 'x') || addrofthis[1] == 'X'))
      {
      aplus += 2; //skip over "0x"
      }
    initscript += "_progfilter = vtk.vtkProgrammableFilter('";
    initscript += aplus;
    initscript += "')\n";
    this->Implementation->Interpretor->RunSimpleString(initscript.c_str());
    // The interpretor has a pointer to the filter therefore there is
    // a reference loop.
    this->Implementation->PythonVarDefined = true;
    }
  
  vtkstd::string runscript;
  // Call the function
  runscript += funcname;
  runscript += "(_progfilter)\n";

  this->Implementation->Interpretor->RunSimpleString(runscript.c_str());

  this->Implementation->Interpretor->FlushMessages();
  //this->Implementation->Interpretor->Delete();
  //this->Implementation->Interpretor = NULL;
}

//----------------------------------------------------------------------------
int vtkPythonProgrammableFilter::FillOutputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int vtkPythonProgrammableFilter::FillInputPortInformation(
  int port, vtkInformation *info)
{
  if(!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }
  if(port==0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
    info->Set(vtkAlgorithm::INPUT_IS_REPEATABLE(), 1);
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    }
  return 1;
}

//----------------------------------------------------------------------------
void vtkPythonProgrammableFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "OutputDataSetType: " << this->OutputDataSetType << endl;
}
