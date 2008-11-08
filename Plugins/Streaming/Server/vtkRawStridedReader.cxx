/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkRawStridedReader.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRawStridedReader.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkByteSwap.h"
#include "vtkExtentTranslator.h"

#include <iostream.h>
#include <fstream>
#include <time.h>
#include <sstream>
#include <string>
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkRawStridedReader, "$Revision: 1.1 $");
vtkStandardNewMacro(vtkRawStridedReader);

#define DEBUGPRINT_STRIDED_READER_DETAILS(arg)\
  ;
#define DEBUGPRINT_STRIDED_READER(arg)\
  ;
#define DEBUGPRINT_METAINFORMATION(arg)\
  ;

//============================================================================
struct vtkRangeRecord2
{
  int p;
  int np;
  double range[2];
};

class vtkRangeKeeper2
{
public:
  vtkRangeKeeper2() 
  {
  }

  ~vtkRangeKeeper2() {
    vtkstd::vector<vtkRangeRecord2 *>::iterator rit;
    for (rit = ranges.begin(); rit < ranges.end(); rit++)
      {
      delete *rit;
      }
    ranges.clear();
  }

 void Insert(int p, int np, double range[2])
  {
    DEBUGPRINT_METAINFORMATION(
    cerr << "Insert range of "
         << range[0] << " .. " << range[1] << " for "
         << p << "/" << np << endl;
                              );

    vtkstd::vector<vtkRangeRecord2 *>::iterator rit;
    for (rit = ranges.begin(); rit < ranges.end(); rit++)
      {
      vtkRangeRecord2 *rr = *rit;
      if (rr->p == p &&
          rr->np == np)
        {
        DEBUGPRINT_METAINFORMATION(cerr << "Found match!" << endl;);
        rr->range[0] = range[0];
        rr->range[1] = range[1];
        return;
        }
      }
        
    DEBUGPRINT_METAINFORMATION(
    cerr << "Inserting new record" << endl;
                               );
    vtkRangeRecord2 *rr = new vtkRangeRecord2();
    rr->p = p;
    rr->np = np;
    rr->range[0] = range[0];
    rr->range[1] = range[1];
    ranges.push_back(rr);
  }

  int Search(int p, int np, double *range)
  {
    DEBUGPRINT_METAINFORMATION(cerr << "Search " <<p << "/" << np << endl;);
    vtkstd::vector<vtkRangeRecord2 *>::iterator rit;
    for (rit = ranges.begin(); rit < ranges.end(); rit++)
      {
      vtkRangeRecord2 *rr = *rit;
      if (rr->p == p &&
          rr->np == np)
        {
        DEBUGPRINT_METAINFORMATION(cerr << "Found match!" << endl;);
        range[0] = rr->range[0];
        range[1] = rr->range[1];
        return 1;
        }
      }
    DEBUGPRINT_METAINFORMATION(cerr << "No match" << endl;);
    return 0;
  }

  vtkstd::vector<vtkRangeRecord2 *> ranges;
};

//==============================================================================

class vtkRawStridedReaderPiece
{
 protected:

 public:
  // ctors and dtors
  vtkRawStridedReaderPiece();
  virtual ~vtkRawStridedReaderPiece();

  // Read a piece.  Extents, strides, and overall data dims must be set
  int read(ifstream& file, unsigned int* strides);

  // Grab the data array (output) pointer
  float* get_data() {return data_;}

  unsigned int get_data_size() {return data_size_;}
  
  void set_buffer_size(unsigned int buffer_size);
  void set_buffer_pointer(float *externalmem);
  void set_uExtents(unsigned int* extents);
  void set_dims(unsigned int* dims);
  void setup_timer() {use_timer_ = true;}
  void swap_endian();
 private:
  unsigned int alloc_data();
  // This prototype needs to change.  Just a placeholder
  unsigned int read_line(ifstream& file,
                         char* cache_buffer,
                         unsigned int buffer_size,
                         unsigned int stride,
                         unsigned int total_bytes,
                         unsigned int insert_at);
  
  bool SwapEndian_;
  unsigned int uExtents_[6];
  unsigned int stride_[3];
  unsigned int dims_[3];
  float* cache_buffer_;
  float* data_;
  float* buffer_pointer_;
  unsigned int output_dims_[3];
  unsigned int output_extents_[6];
  unsigned int buffer_size_;
  unsigned int data_size_;

  bool use_timer_;
  clock_t start;
  clock_t stop;  
};


// Default ctor.  Some simple initialization
vtkRawStridedReaderPiece::vtkRawStridedReaderPiece() : data_(0), cache_buffer_(NULL), use_timer_(false), SwapEndian_(false), buffer_pointer_(NULL)
{ }

// dtor - handles data cleanup
vtkRawStridedReaderPiece::~vtkRawStridedReaderPiece()
{
  if (data_ && buffer_pointer_ != data_ )
  {
    delete[] data_;
    data_ = 0;
  }
  if(cache_buffer_ != NULL)
  {
    delete[] cache_buffer_;
    cache_buffer_ = NULL;
  }
}

void vtkRawStridedReaderPiece::swap_endian()
{
  if (SwapEndian_)
  {
    SwapEndian_ = false;
  }
  else
  {
    SwapEndian_ = true;
  }
}

void vtkRawStridedReaderPiece::set_buffer_size(unsigned int size)
{
  buffer_size_ = size;
}

void vtkRawStridedReaderPiece::set_buffer_pointer(float *external_mem)
{
  if (data_ && buffer_pointer_ != data_ )
    {
    delete[] data_;
    }
  buffer_pointer_ = external_mem;
  data_ = buffer_pointer_;
}

/*  Set the extents of the piece to grab, in scaled down coords.*/
void vtkRawStridedReaderPiece::set_uExtents(unsigned int* extents)
{
  for(int i = 0; i < 6; ++i)
    uExtents_[i] = extents[i];
}

/*  Set the dimensions of the dataset as a whole.  This is necessary
    to calculate the proper seeks and offsets that must be used */
void vtkRawStridedReaderPiece::set_dims(unsigned int* dims)
{
  for(int i = 0; i < 3; ++i)
    dims_[i] = dims[i];
}

/*  Allocate the final data array.  The data array will be the exact
    size necessary to accomodate all elements of the output.  Return
    the number of elements in the output (float) array. */
unsigned int vtkRawStridedReaderPiece::alloc_data()
{
  unsigned int i_span = uExtents_[1] - uExtents_[0] + 1;
  unsigned int j_span = uExtents_[3] - uExtents_[2] + 1;
  unsigned int k_span = uExtents_[5] - uExtents_[4] + 1;
  
  DEBUGPRINT_STRIDED_READER_DETAILS(
  cerr << "output dims: " 
       << i_span << ", " << j_span << ", " << k_span << endl;
                           );

  data_size_ = i_span * j_span * k_span;
  if (data_ && buffer_pointer_ != data_ )
  {
    delete[] data_;
  }

  if (buffer_pointer_ != NULL)
    {
    data_ = buffer_pointer_;
    }
  else
    {
    data_ = new float[data_size_];
    if (data_ == 0)
      {
      cerr << "NEW FAILURE" << endl;
      }
    }
  
  if (cache_buffer_ != NULL)
    {
    //cerr << "Free old cache_buffer_" << endl;
    delete[] cache_buffer_;
    }
  cache_buffer_ = new float[buffer_size_/sizeof(float)];
  if (cache_buffer_ == 0)
    {
    cerr << "NEW FAILURE" << endl;
    }

  return data_size_;
}

/*  Read a scanline of the data in the dimension changing fastest. */
unsigned int vtkRawStridedReaderPiece::read_line(ifstream& file,
                              char* cache_buffer,
                              unsigned int buffer_size,
                              unsigned int stride,
                              unsigned int bytes_to_read,
                              unsigned int insert_at)
{
  DEBUGPRINT_STRIDED_READER_DETAILS(
                                    cerr << "stride " << stride << " B2R " << bytes_to_read << endl;
                                    );

  unsigned int read_from = 0;
  unsigned int bytes_read = 0;
  unsigned int vals_read = 0;

  unsigned int vals_in_buffer = buffer_size / sizeof(float);//#bytes to #floats

  //adjust buffer down to hold a whole number of floats
  unsigned int adjusted_buffer_size = buffer_size;  
  if (vals_in_buffer*sizeof(float) < buffer_size)
    {
    adjusted_buffer_size = vals_in_buffer*sizeof(float);
    DEBUGPRINT_STRIDED_READER_DETAILS(
    cerr << "Round " << buffer_size << " to " << adjusted_buffer_size << endl;
                                      );
    }
  unsigned int strided_vals_in_buffer = vals_in_buffer/stride; 

  //check for case when stride is bigger than buffer.
  //In that case adjust truncated fraction from 0 to 1
  if (strided_vals_in_buffer <= 1)
    {
    DEBUGPRINT_STRIDED_READER_DETAILS(
    cerr << "Floor to " << 1 << endl;
                                      );
    strided_vals_in_buffer = 1;  
    adjusted_buffer_size = strided_vals_in_buffer*stride*sizeof(float);
    vals_in_buffer = strided_vals_in_buffer*stride;
    }

  //check for case when buffer is bigger than the number of vals we want.
  //In that case, read only what we want.
  unsigned int sought_vals = (uExtents_[1]-uExtents_[0]+1);
  if (strided_vals_in_buffer > sought_vals)
    {    
    DEBUGPRINT_STRIDED_READER_DETAILS(
    cerr << "Ceiling to " << sought_vals << endl;
                                      );
    strided_vals_in_buffer = sought_vals;
    adjusted_buffer_size = strided_vals_in_buffer*stride*sizeof(float);
    vals_in_buffer = strided_vals_in_buffer*stride;
    }

  // If the number of reads per buffer is 1, then we know our stride
  // is larger than the buffer size.  This means that the most efficient
  // way to read is to seek to the value and do a buffered read.
  if (strided_vals_in_buffer == 1)
    {
    DEBUGPRINT_STRIDED_READER_DETAILS(
    cerr << "single reads " << endl; 
                                      );
    //ifstream::pos_type orig = file.tellg();    
    //unsigned int c=orig;
    while(vals_read < sought_vals)
      { 
      DEBUGPRINT_STRIDED_READER_DETAILS(
      cerr << "READ AT " << file.tellg();     
                                        );
      file.read(cache_buffer, adjusted_buffer_size);
      if (file.bad())
        {
        cerr << "READ FAIL 1" << endl;
        }
      float* float_buffer = cache_buffer_;
      // Since we do a seek, the element will always be at location 0
      data_[insert_at] = float_buffer[0];
      DEBUGPRINT_STRIDED_READER_DETAILS(
      cerr << " " << data_[insert_at] << " ";
                                        );
      insert_at++;
      /*
      // Seek to the next value we want to read! 
      c+=stride*sizeof(float);
      DEBUGPRINT_STRIDED_READER_DETAILS(
      cerr << " seek to " << c << endl;
                                        );
      */
      file.seekg(stride*sizeof(float), ios::cur);//c, ios::beg); //); was buggy
      if (file.bad())
        {
        cerr << "SEEK FAIL" << endl;
        }

      // The number of bytes we read now is not the size of the cache,
      // but it's the size of the stride (in bytes).
      bytes_read += sizeof(float);
      vals_read++;
      }
    } 
  else 
    {
    DEBUGPRINT_STRIDED_READER_DETAILS(
    cerr << "Read " << sought_vals << " values from " 
         << adjusted_buffer_size << " byte buffers that hold "
         << strided_vals_in_buffer << endl;
                                      );
    //Like above we might have a buffer which doesn't cover a whole line,
    //so we loop, reading buffers until we get all the values for the line.
    while(vals_read < sought_vals) 
      {
      //read a buffer
      DEBUGPRINT_STRIDED_READER_DETAILS(
      cerr << "READ AT " << file.tellg() << endl;
                                        );
      file.read(cache_buffer, adjusted_buffer_size);
      if (file.bad())
        {
        cerr << "READ FAIL 2" << endl;
        }
      float* float_buffer = cache_buffer_;
      // Unlike above we now have more than 1 vals in each buffer, 
      // so we need to step through and extract the values that lie on 
      // the stride
      while(read_from < vals_in_buffer)
        {
        data_[insert_at] = float_buffer[read_from];
        DEBUGPRINT_STRIDED_READER_DETAILS(
        cerr << "fbuffer[" << read_from << "]=" 
             << data_[insert_at] << "-> " << insert_at << endl;
                                          );
        insert_at++;
        read_from+=stride;
        vals_read++;
        bytes_read += sizeof(float);
        //multiple small buffers can overrun, so detect and finish
        if (vals_read == sought_vals)
          {
          break;
          }
        }
      //when loop, we careful to start at right place in buffer
      read_from = read_from % vals_in_buffer;
      }
    }
  
  DEBUGPRINT_STRIDED_READER_DETAILS(
  cerr << "Bytes read " << bytes_read << endl;
                                    );
  return insert_at;
}


/*  Read a piece off disk.  This must be done AFTER definiing the
    extents and dimensions.  The read also assumes that the data
    is organized on disk with the fastest changing dimension
    specified first, followed sequential by the 2 next fastest
    dimensions.  Here, it is organized as row-major, x,y,z format.
*/
int vtkRawStridedReaderPiece::read(ifstream& file,
                 unsigned int* strides)
{
  
  if(use_timer_)
  {
    start = clock();
  }
  
  for(int i = 0; i < 3; ++i)
  {
    if(strides[i] == 0)
      {
      cerr << "Cannot read a piece with a stride of 0." << endl;
      return 0;
      }
    stride_[i] = strides[i];
  }
  if (buffer_size_ < sizeof(float))
    {
    cerr << "buffer size must be a multiple of " << sizeof(float) << endl;
    return 0;
    }
  DEBUGPRINT_STRIDED_READER_DETAILS(unsigned int num_floats =)
    alloc_data();
  DEBUGPRINT_STRIDED_READER_DETAILS(cerr << "We have room for " << num_floats << " floats." << endl;);

  unsigned int insert_index = 0;
  
  // Size in floats!  We have to multiply by sizeof(float) to get the bytes.
  // But that will happen just a little bit later!
  unsigned int plane_size = dims_[1] * dims_[0];
  unsigned int row_size = dims_[0];
  unsigned int bytes_to_read = (uExtents_[1]-uExtents_[0]+1) * sizeof(float);
  DEBUGPRINT_STRIDED_READER_DETAILS(
  cerr << "plane size = " << plane_size << " row size = " << row_size << " b2r = " << bytes_to_read << endl;
                           );
  
  for(unsigned int k = uExtents_[4]; k <= uExtents_[5]; k++)
  {
    for(unsigned int j = uExtents_[2]; j <= uExtents_[3]; j++)
    {
    unsigned int i = uExtents_[0];
    unsigned int offset = 
      k*strides[2]*plane_size*sizeof(float) + 
      j*strides[1]*row_size*sizeof(float) + 
      i*strides[0]*sizeof(float);

    DEBUGPRINT_STRIDED_READER_DETAILS(
    cerr << "read line " 
         << k << "," << j 
         << "(" << k*strides[2] <<","<<j*strides[1] << ")" 
         << " from " << offset << endl;
                                      );
    // Seek to the beginning of the line we want to extract.
    // The file pointer is now at the first element of the
    // extent to extract.
    file.seekg(offset, ios::beg);
    if (file.bad())
      {
      cerr << "SEEK FAIL" << endl;
      return 0;
      }

    // Extract the line.  To do this we need to know the stride
    // and the last extent to grab, as well as the location
    // of the output array and the position to put stuff in there.
    insert_index = 
      read_line(file, 
                (char*)cache_buffer_, buffer_size_, 
                strides[0], 
                bytes_to_read, 
                insert_index);    
    }
  }
  DEBUGPRINT_STRIDED_READER_DETAILS(
  cerr << "Read " << insert_index << " floats total " << endl;
                                    );
  
  if(use_timer_)
  {
    stop = clock();
    double t = stop - start;
    double elapsed = t / CLOCKS_PER_SEC;
    cerr << "Took " << elapsed << " seconds to read." << endl;
  }

  if (SwapEndian_)
  {
  vtkByteSwap::SwapVoidRange(data_, insert_index , sizeof(float) );
  }
  return 1;
}

//============================================================================

vtkRawStridedReader::vtkRawStridedReader()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->Internal = new vtkRawStridedReaderPiece();
  this->Filename = NULL;
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = 99;
  this->Dimensions[0] = this->WholeExtent[1] - this->WholeExtent[0] +1;
  this->Dimensions[1] = this->WholeExtent[3] - this->WholeExtent[2] +1;
  this->Dimensions[2] = this->WholeExtent[5] - this->WholeExtent[4] +1;
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 1.0;
  this->Stride[0] = this->Stride[1] = this->Stride[2] = 1;
  this->BlockReadSize = sizeof(float)*1024*1024;
  this->RangeKeeper = new vtkRangeKeeper2();
}

//----------------------------------------------------------------------------
vtkRawStridedReader::~vtkRawStridedReader()
{
  if (this->Filename)
    {
    delete[] this->Filename;
    }
  delete this->Internal;
  delete this->RangeKeeper;
}

//----------------------------------------------------------------------------
void vtkRawStridedReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


//----------------------------------------------------------------------------
void vtkRawStridedReader::SwapDataByteOrder(int i)
{
  if (i == 1)
    {
    this->Internal->swap_endian();
    }
}

//----------------------------------------------------------------------------
//RequestInformation supplies global meta information
// Global Extents  (integer count range of point count in x,y,z)
// Global Origin 
// Global Spacing (should be the stride value * original)

int vtkRawStridedReader::RequestInformation(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{ 
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

//  cerr << "Stride: "
//       << this->Stride[0] << "," 
//       << this->Stride[1] << "," 
//       << this->Stride[2] << endl;
  DEBUGPRINT_STRIDED_READER(
  cerr << "Whole extent: " 
       << this->WholeExtent[0] << ".." << this->WholeExtent[1] << ","
       << this->WholeExtent[2] << ".." << this->WholeExtent[3] << ","
       << this->WholeExtent[4] << ".." << this->WholeExtent[5] << endl;
                           );
  this->Dimensions[0] = this->WholeExtent[1] - this->WholeExtent[0]+1;
  this->Dimensions[1] = this->WholeExtent[3] - this->WholeExtent[2]+1;
  this->Dimensions[2] = this->WholeExtent[5] - this->WholeExtent[4]+1;
  DEBUGPRINT_STRIDED_READER(
  cerr << "Dimensions: " 
       << this->Dimensions[0] << ","
       << this->Dimensions[1] << ","
       << this->Dimensions[2] << endl;
                           );  

  int sWext[6];
  sWext[0] = this->WholeExtent[0];
  sWext[1] = this->WholeExtent[1] / this->Stride[0];
  sWext[2] = this->WholeExtent[2];
  sWext[3] = this->WholeExtent[3] / this->Stride[1];
  sWext[4] = this->WholeExtent[4];
  sWext[5] = this->WholeExtent[5] / this->Stride[2];

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), sWext ,6);

  outInfo->Set(vtkDataObject::ORIGIN(), this->Origin, 3);

  double stridedSpacing[3];
  for (int i = 0; i < 3; i++)
    {
    stridedSpacing[i] = this->Spacing[i]*this->Stride[i];
    }
  outInfo->Set(vtkDataObject::SPACING(), stridedSpacing, 3);
  DEBUGPRINT_STRIDED_READER(
  cerr << "Spacing "
       << this->Spacing[0] << "," 
       << this->Spacing[1] << "," 
       << this->Spacing[2] << endl;
                           );

  double bounds[6];
  bounds[0] = this->Origin[0] + stridedSpacing[0] * sWext[0];
  bounds[1] = this->Origin[0] + stridedSpacing[0] * sWext[1];
  bounds[2] = this->Origin[1] + stridedSpacing[1] * sWext[2];
  bounds[3] = this->Origin[1] + stridedSpacing[1] * sWext[3];
  bounds[4] = this->Origin[2] + stridedSpacing[2] * sWext[4];
  bounds[5] = this->Origin[2] + stridedSpacing[2] * sWext[5];

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),
               bounds, 6);

  vtkDataObject::SetPointDataActiveScalarInfo(outInfo, VTK_FLOAT, 1);

  return 1;
}

//----------------------------------------------------------------------------
// Here unlike the RequestInformation we getting, not setting 
int vtkRawStridedReader::RequestUpdateExtent(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
               this->UpdateExtent);  

  DEBUGPRINT_STRIDED_READER(
  int P = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int NP = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  cerr << "RSR(" << this << ") Strided uExt "
       << P << "/" << NP << " = "
       << this->UpdateExtent[0] << ".." << this->UpdateExtent[1] << ","
       << this->UpdateExtent[2] << ".." << this->UpdateExtent[3] << ","
       << this->UpdateExtent[4] << ".." << this->UpdateExtent[5] << endl;
                           );
  return 1;
}

//----------------------------------------------------------------------------
int vtkRawStridedReader::RequestData(
    vtkInformation* vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed(inputVector),
    vtkInformationVector* outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *outData = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!outData)
    {
    cerr << "Wrong output type" << endl;
    return 0;
    }
  if (!this->Filename)
    {
    cerr << "Must specify filename" << endl;
    return 0;
    }
  outData->Initialize();

  vtkInformation *dInfo = outData->GetInformation();

  //prepping to produce real data and thus allocate real amounts of space
  int *uext = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  outData->SetExtent(uext);

  //Todo We need to be able to have user definable data type
  //Todo also multiple arrays/multiple scalarComponents
  outData->AllocateScalars();
  outData->GetPointData()->GetScalars()->SetName("PointCenteredData");
  float *myfloats = (float*)outData->GetScalarPointer();

  ifstream file(this->Filename, ios::in|ios::binary);
  if(!file.is_open())
  {
    cerr << "Could not open file: " << this->Filename << endl;
    return 0;
  }
  if (file.bad())
    {
    cerr << "OPEN FAIL" << endl;
    return 0;
    }

  this->Internal->set_uExtents((unsigned int*)uext);
  this->Internal->set_dims((unsigned int*)this->Dimensions);
  this->Internal->set_buffer_size((unsigned int)this->BlockReadSize);
  this->Internal->set_buffer_pointer(myfloats);

  //if(use_timer.compare("1") == 0)
  //  this->Internal->setup_timer();
  
  int rc = this->Internal->read(file, (unsigned int*)this->Stride);
  if (!rc)
    {
    cerr << "READ FAIL 3" << endl;
    return 0;
    }

  DEBUGPRINT_STRIDED_READER(
  unsigned int memsize = this->Internal->get_data_size(); 
  cerr << "memsize " << memsize << endl;
                           );

  file.close();

  double range[2];
  outData->GetPointData()->GetScalars()->GetRange(range);
  int P = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int NP = outInfo->Get(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  DEBUGPRINT_METAINFORMATION(
  cerr << "RSR(" << this << ") Calculate range " << range[0] << ".." << range[1] << " for " << P << "/" << NP << endl;
                             );
  this->RangeKeeper->Insert(P, NP, range);

  return 1;
}

//----------------------------------------------------------------------------
int vtkRawStridedReader::ProcessRequest(vtkInformation *request,
                   vtkInformationVector **inputVector,
                   vtkInformationVector *outputVector)
{

//cerr << "RSR--------------------------------------------------------------" << endl;
//vtkPrintit(request);
//vtkPrintit(outputVector->GetInformationObject(0));
//cerr << "-------------------------------------------------------------------" << endl;
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
    {
    DEBUGPRINT_STRIDED_READER(cerr << "RSR(" << this << ") RDO =====================================" << endl;);
    }

  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    DEBUGPRINT_STRIDED_READER(cerr << "RSR(" << this << ") RI =====================================" << endl;);
    }

  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    DEBUGPRINT_STRIDED_READER(cerr << "RSR(" << this << ") RUE =====================================" << endl;);
    }

  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT_INFORMATION()))
    {
    DEBUGPRINT_METAINFORMATION(cerr << "RSR(" << this << ") RUEI =====================================" << endl;);
    //create meta information for this piece
    double *origin;
    double *spacing;
    int *ext;
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    origin = outInfo->Get(vtkDataObject::ORIGIN());
    spacing = outInfo->Get(vtkDataObject::SPACING());
    ext = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());    
    int P = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    int NP = outInfo->Get(
      vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    double bounds[6];
    bounds[0] = origin[0] + spacing[0] * ext[0];
    bounds[1] = origin[0] + spacing[0] * ext[1];
    bounds[2] = origin[1] + spacing[1] * ext[2];
    bounds[3] = origin[1] + spacing[1] * ext[3];
    bounds[4] = origin[2] + spacing[2] * ext[4];
    bounds[5] = origin[2] + spacing[2] * ext[5];
    outInfo->Set(vtkStreamingDemandDrivenPipeline::PIECE_BOUNDING_BOX(), bounds, 6);
    DEBUGPRINT_METAINFORMATION(
      cerr << "For " 
      << "P" << "/" << "NP" << "\tB=" 
      << bounds[0] << "," << bounds[1] << ","
      << bounds[2] << "," << bounds[3] << ","
      << bounds[4] << "," << bounds[5] << "\t"
                                 );

    double range[2];
    if (this->RangeKeeper->Search(P, NP, range))
      {
      DEBUGPRINT_METAINFORMATION(
      cerr << "R=" << range[0] << " .. " << range[1] << endl;
                                 );
      vtkInformation *fInfo = 
        vtkDataObject::GetActiveFieldInformation
        (outInfo, vtkDataObject::FIELD_ASSOCIATION_POINTS, 
         vtkDataSetAttributes::SCALARS);
      if (fInfo)
        {
        fInfo->Set(vtkDataObject::PIECE_FIELD_RANGE(), range, 2);
        }
      }
    else
      {
      DEBUGPRINT_METAINFORMATION(
      cerr << "No range for " 
           << P << "/" << NP << " "
           << " yet" << endl;        
                                 );
      }
        
    }

  //This is overridden just to intercept requests for debugging purposes.
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    DEBUGPRINT_STRIDED_READER(cerr << "RSR(" << this << ") RD =====================================" << endl;);
    
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    int updateExtent[6];
    int wholeExtent[6];
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), 
      updateExtent);
    outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), 
      wholeExtent);

    bool match = true;
    for (int i = 0; i< 6; i++)
      {
        if (updateExtent[i] != wholeExtent[i])
          {
          match = false;
          }
      }
    if (match)
      {
      cerr << "WARNING: Whole extent requested from " << this->GetClassName() << endl;
      }

    }

  int rc = this->Superclass::ProcessRequest(request, inputVector, outputVector);
  return rc;
}


