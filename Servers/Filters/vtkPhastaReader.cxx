/*=========================================================================

Program:   Visualization Toolkit
Module:    $RCSfile: vtkPhastaReader.cxx,v $

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPhastaReader.h"

#include "vtkByteSwap.h"
#include "vtkCellType.h"   //added for constants such as VTK_TETRA etc...
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkPhastaReader, "$Revision: 1.2 $");
vtkStandardNewMacro(vtkPhastaReader);

#define swap_char(A,B) { ucTmp = A; A = B ; B = ucTmp; }

// Begin of copy from phastaIO

#include <vtkstd/map>
#include <vtkstd/vector>
#include <vtkstd/string>

vtkstd::map< int , char* > LastHeaderKey;
vtkstd::vector< FILE* > fileArray;
vtkstd::vector< int > byte_order;
vtkstd::vector< int > header_type;
int DataSize=0;
int LastHeaderNotFound = 0;
int Wrong_Endian = 0 ;
int Strict_Error = 0 ;
int binary_format = 0;


// the caller has the responsibility to delete the returned string 

char* vtkPhastaReader::StringStripper( const char  istring[] ) 
{
  int length = strlen( istring );
  char* dest = new char [ length + 1 ];
  strcpy( dest, istring );
  dest[ length ] = '\0';

  if ( char* p = strpbrk( dest, " ") ) 
    {
    *p = '\0';
    }
        
  return dest;
}

int vtkPhastaReader::cscompare( const char teststring[], 
                              const char targetstring[] ) 
{

  char* s1 = const_cast<char*>(teststring);
  char* s2 = const_cast<char*>(targetstring);

  while( *s1 == ' ') { s1++; }
  while( *s2 == ' ') { s2++; }
  while( ( *s1 ) 
         && ( *s2 ) 
         && ( *s2 != '?')
         && ( tolower( *s1 )==tolower( *s2 ) ) ) 
    {
    s1++;
    s2++;
    while( *s1 == ' ') { s1++; }
    while( *s2 == ' ') { s2++; }
    }
  if ( !( *s1 ) || ( *s1 == '?') ) 
    {
    return 1;
    }
  else 
    {
    return 0;
    }
}
    
void vtkPhastaReader::isBinary( const char iotype[] ) 
{
  char* fname = StringStripper( iotype );
  if ( cscompare( fname, "binary" ) ) 
    {
    binary_format = 1;
    }
  else 
    {
    binary_format = 0;
    }
  delete [] fname;

}

size_t vtkPhastaReader::typeSize( const char typestring[] ) 
{
  char* ts1 = StringStripper( typestring );

  if ( cscompare( "integer", ts1 ) ) {
  delete [] ts1;
  return sizeof(int);
  } else if ( cscompare( "double", ts1 ) ) { 
  delete [] ts1;
  return sizeof( double );
  } else { 
  delete [] ts1;
  fprintf(stderr,"unknown type : %s\n",ts1);
  return 0;
  }
}
    
int vtkPhastaReader::readHeader( FILE*       fileObject,
                               const char  phrase[],
                               int*        params,
                               int         expect ) 
{
  char* text_header;
  char* token;
  char Line[1024];
  char junk;
  int FOUND = 0 ;
  int real_length;
  int skip_size, integer_value;
  int rewind_count=0;

  if( !fgets( Line, 1024, fileObject ) && feof( fileObject ) ) 
    {
    rewind( fileObject );
    clearerr( fileObject );
    rewind_count++;
    fgets( Line, 1024, fileObject );
    }
        
  while( !FOUND  && ( rewind_count < 2 ) )  
    {
    if ( ( Line[0] != '\n' ) && ( real_length = strcspn( Line, "#" )) )
      {
      text_header = new char [ real_length + 1 ];
      strncpy( text_header, Line, real_length );
      text_header[ real_length ] =static_cast<char>(NULL);
      token = strtok ( text_header, ":" );
      if( cscompare( phrase , token ) ) 
        {
        FOUND = 1 ;
        token = strtok( NULL, " ,;<>" );
        skip_size = atoi( token );
        int i;
        for( i=0; i < expect && ( token = strtok( NULL," ,;<>") ); i++) 
          {
          params[i] = atoi( token );
          }
        if ( i < expect ) 
          {
          fprintf(stderr,"Expected # of ints not found for: %s\n",phrase );
          }
        } 
      else if ( cscompare(token,"byteorder magic number") ) 
        {
        if ( binary_format ) 
          {
          fread((void*)&integer_value,sizeof(int),1,fileObject);
          fread( &junk, sizeof(char), 1 , fileObject );
          if ( 362436 != integer_value ) 
            {
            Wrong_Endian = 1;
            }
          } 
        else
          {
          fscanf(fileObject, "%d\n", &integer_value );
          }
        } 
      else 
        { 
        /* some other header, so just skip over */
        token = strtok( NULL, " ,;<>" );
        skip_size = atoi( token );
        if ( binary_format) 
          {
          fseek( fileObject, skip_size, SEEK_CUR );
          }
        else 
          {
          for( int gama=0; gama < skip_size; gama++ ) 
            {
            fgets( Line, 1024, fileObject );
            }
          }
        }
      delete [] text_header;
      }

    if ( !FOUND ) 
      {
      if( !fgets( Line, 1024, fileObject ) && feof( fileObject ) ) 
        {
        rewind( fileObject );
        clearerr( fileObject );
        rewind_count++;
        fgets( Line, 1024, fileObject );
        }
      }
    }             
        
  if ( !FOUND ) 
    {
    fprintf(stderr, "Error: Cound not find: %s\n", phrase);
    return 1;
    }
  return 0;
}       

void vtkPhastaReader::SwapArrayByteOrder( void* array, 
                                         int   nbytes, 
                                         int   nItems ) 
{
  /* This swaps the byte order for the array of nItems each
     of size nbytes , This will be called only locally  */
  int i,j;
  unsigned char ucTmp;
  unsigned char* ucDst = (unsigned char*)array;
        
  for(i=0; i < nItems; i++) 
    {
    for(j=0; j < (nbytes/2); j++)
      {
      swap_char( ucDst[j] , ucDst[(nbytes - 1) - j] );
      }
    ucDst += nbytes;
    }
}
    

void vtkPhastaReader::openfile( const char filename[],
                              const char mode[],
                              int*  fileDescriptor ) 
{
  FILE* file=NULL ;
  *fileDescriptor = 0;
  char* fname = StringStripper( filename );
  char* imode = StringStripper( mode );

  if ( cscompare( "read", imode ) ) 
    {
    file = fopen(fname, "rb" );
    }
  else if( cscompare( "write", imode ) ) 
    {
    file = fopen(fname, "wb" );
    }
  else if( cscompare( "append", imode ) ) 
    {
    file = fopen(fname, "ab" );
    }
    
  if ( !file )
    {
    fprintf(stderr,"unable to open file : %s\n",fname ) ;
    } 
  else 
    {
    fileArray.push_back( file );
    byte_order.push_back( 0 );         
    header_type.push_back( sizeof(int) );
    *fileDescriptor = fileArray.size();
    }
  delete [] fname;
  delete [] imode;
}

void vtkPhastaReader::closefile( int* fileDescriptor, 
                                const char mode[] ) 
{
  char* imode = StringStripper( mode );

  if( cscompare( "write", imode ) || 
      cscompare( "append", imode ) ) 
    {
    fflush( fileArray[ *fileDescriptor - 1 ] );
    } 

  fclose( fileArray[ *fileDescriptor - 1 ] );
  delete [] imode;
}

void vtkPhastaReader::readheader( int* fileDescriptor,
                                const char keyphrase[],
                                void* valueArray,
                                int*  nItems,
                                const char  datatype[],
                                const char  iotype[] ) 
{
  int filePtr = *fileDescriptor - 1;
  FILE* fileObject;
  int* valueListInt;

  if ( *fileDescriptor < 1 || *fileDescriptor > (int)fileArray.size() ) 
    {
    fprintf(stderr,"No file associated with Descriptor %d\n",*fileDescriptor);
    fprintf(stderr,"openfile function has to be called before \n") ;
    fprintf(stderr,"acessing the file\n ") ;
    fprintf(stderr,"fatal error: cannot continue, returning out of call\n");
    return;
    }

  LastHeaderKey[ filePtr ] = const_cast< char* >( keyphrase ); 
  LastHeaderNotFound = 0;

  fileObject = fileArray[ filePtr ] ;
  Wrong_Endian = byte_order[ filePtr ];

  isBinary( iotype );
  typeSize( datatype );   //redundant call, just avoid a compiler warning.

  // right now we are making the assumption that we will only write integers
  // on the header line.

  valueListInt = static_cast< int* >( valueArray );
  int ierr = readHeader( fileObject ,
                         keyphrase,
                         valueListInt,
                         *nItems ) ;

  byte_order[ filePtr ] = Wrong_Endian ;

  if ( ierr ) 
    {
    LastHeaderNotFound = 1;
    }
    
  return ;
}

void vtkPhastaReader::readdatablock( int*  fileDescriptor,
                                   const char keyphrase[],
                                   void* valueArray,
                                   int*  nItems,
                                   const char  datatype[],
                                   const char  iotype[] ) 
{    
  int filePtr = *fileDescriptor - 1;
  FILE* fileObject;
  char junk;
    
  if ( *fileDescriptor < 1 || *fileDescriptor > (int)fileArray.size() ) 
    {
    fprintf(stderr,"No file associated with Descriptor %d\n",*fileDescriptor);
    fprintf(stderr,"openfile function has to be called before \n") ;
    fprintf(stderr,"acessing the file\n ") ;
    fprintf(stderr,"fatal error: cannot continue, returning out of call\n");
    return;
    }
    
  // error check..
  // since we require that a consistant header always preceed the data block
  // let us check to see that it is actually the case.    

  if ( ! cscompare( LastHeaderKey[ filePtr ], keyphrase ) ) 
    {
    fprintf(stderr, "Header not consistant with data block\n");
    fprintf(stderr, "Header: %s\n", LastHeaderKey[ filePtr ] );
    fprintf(stderr, "DataBlock: %s\n ", keyphrase ); 
    fprintf(stderr, "Please recheck read sequence \n");
    if( Strict_Error ) 
      {
      fprintf(stderr, "fatal error: cannot continue, returning out of call\n"); 
      return;
      }
    }
    
  if ( LastHeaderNotFound ) { return; }

  fileObject = fileArray[ filePtr ];
  Wrong_Endian = byte_order[ filePtr ];

  size_t type_size = typeSize( datatype ); 
  int nUnits = *nItems;
  isBinary( iotype );
    
  if ( binary_format ) 
    {
    fread( valueArray, type_size, nUnits, fileObject );
    fread( &junk, sizeof(char), 1 , fileObject );
    if ( Wrong_Endian ) 
      {
      SwapArrayByteOrder( valueArray, type_size, nUnits );
      }
    } 
  else 
    { 
    char* ts1 = StringStripper( datatype );
    if ( cscompare( "integer", ts1 ) ) 
      {
      for( int n=0; n < nUnits ; n++ ) 
        {
        fscanf(fileObject, "%d\n",(int*)((int*)valueArray+n) );  
        }
      } 
    else if ( cscompare( "double", ts1 ) ) 
      {
      for( int n=0; n < nUnits ; n++ )
        {
        fscanf(fileObject, "%lf\n",(double*)((double*)valueArray+n) );
        }
      }
    delete [] ts1;
    }
    
  return;
}


// End of copy from phastaIO


vtkPhastaReader::vtkPhastaReader()
{
  this->GeometryFileName = NULL;
  this->FieldFileName = NULL;
  this->SetNumberOfInputPorts(0);
}

vtkPhastaReader::~vtkPhastaReader()
{
  if (this->GeometryFileName)
    {
    delete [] this->GeometryFileName;
    }
  if (this->FieldFileName)
    {
    delete [] this->FieldFileName;
    }
}

int vtkPhastaReader::RequestData(vtkInformation*,
                               vtkInformationVector**,
                               vtkInformationVector* outputVector)
{
  int firstVertexNo = 0;
  int fvn = 0;
  int noOfNodes;

  // get the data object
  vtkInformation *outInfo = 
    outputVector->GetInformationObject(0);

  vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPoints *points;

  output->Allocate(10000, 2100);

  vtkDataSetAttributes* field = output->GetPointData();

  points = vtkPoints::New();

  vtkDebugMacro(<<"Reading Phasta file...");

  if(!this->GeometryFileName || !this->FieldFileName )
    {
    vtkErrorMacro(<<"All input parameters not set.");
    return 0;
    }
  vtkDebugMacro(<< "Updating ensa with ....");
  vtkDebugMacro(<< "Geom File : " << this->GeometryFileName);
  vtkDebugMacro(<< "Field File : " << this->FieldFileName);

  fvn = firstVertexNo;
  this->ReadGeomFile(this->GeometryFileName, firstVertexNo, points, noOfNodes);
  this->ReadFieldFile(this->FieldFileName, fvn, field, noOfNodes);

  /* set the points over here, this is because vtkUnStructuredGrid 
     only insert points once, next insertion overwrites the previous one */

  output->SetPoints(points);
  points->Delete();

  return 1;
}

/* firstVertexNo is useful when reading multiple geom files and coalescing 
   them into one, ReadGeomfile can then be called repeatedly from Execute with 
   firstVertexNo forming consecutive series of vertex numbers */

void vtkPhastaReader::ReadGeomFile(char* geomFileName, 
                                 int &firstVertexNo, 
                                 vtkPoints *points,
                                 int &num_nodes)
{

  /* variables for vtk */
  vtkUnstructuredGrid *output = this->GetOutput();
  double *coordinates;
  vtkIdType *nodes;
  int cell_type;

  //  int num_tpblocks;
  /* variables for the geom data file */
  /* nodal information */
  // int byte_order; 
  //int data[11], data1[7];
  int dim;
  int num_int_blocks;
  double *pos;
  //int *nlworkdata;
  /* element information */ 
  int num_elems,num_vertices,num_per_line;
  int *connectivity;


  /* misc variables*/
  int i, j,k,item;
  int geomfile;

  openfile(geomFileName,"read",&geomfile);
  //geomfile = fopen(GeometryFileName,"rb");

  if(!geomfile)
    {
    vtkErrorMacro(<<"Cannot open file " << geomFileName);
    return;
    }

  int expect;
  int array[10];
  expect = 1;

  /* read number of nodes */

  readheader(&geomfile,"number of nodes",array,&expect,"integer","binary");
  num_nodes = array[0];

  /* read number of elements */
  readheader(&geomfile,
             "number of interior elements",
             array,
             &expect,
             "integer",
             "binary");
  num_elems = array[0];

  /* read number of interior */
  readheader(&geomfile,
             "number of interior tpblocks",
             array,
             &expect,
             "integer",
             "binary");
  num_int_blocks = array[0];

  vtkDebugMacro ( << "Nodes: " << num_nodes
                  << "Elements: " << num_elems
                  << "tpblocks: " << num_int_blocks );

  /* read coordinates */
  expect = 2;
  readheader(&geomfile,"co-ordinates",array,&expect,"integer","binary");
  // TEST *******************
  num_nodes=array[0];
  // TEST *******************
  if(num_nodes !=array[0])
    {
    vtkErrorMacro(<<"Ambigous information in geom.data file, number of nodes does not match the co-ordinates size. Nodes: " << num_nodes << " Coordinates: " << array[0]);
    return;
    }
  dim = array[1];

  
  /* read the coordinates */

  coordinates = new double [dim];
  if(coordinates == NULL)
    {
    vtkErrorMacro(<<"Unable to allocate memory for nodal info");
    return;
    }

  pos = new double [num_nodes*dim];
  if(pos == NULL)
    {
    vtkErrorMacro(<<"Unable to allocate memory for nodal info");
    return;
    }
  
  item = num_nodes*dim;
  readdatablock(&geomfile,"co-ordinates",pos,&item,"double","binary");

  for(i=0;i<num_nodes;i++)
    {
    for(j=0;j<dim;j++)
      {
      coordinates[j] = pos[j*num_nodes + i];
      }
    switch(dim)
      {
      case 1:
        points->InsertPoint(i+firstVertexNo,coordinates[0],0,0);
        break;
      case 2:
        points->InsertPoint(i+firstVertexNo,coordinates[0],coordinates[1],0);
        break; 
      case 3:
        points->InsertNextPoint(coordinates);
        break;
      default:
        vtkErrorMacro(<<"Unrecognized dimension in "<< geomFileName)
          return;
      }
    }

  /* read the connectivity information */
  expect = 7;

  for(k=0;k<num_int_blocks;k++)
    {
    readheader(&geomfile,
               "connectivity interior",
               array,
               &expect,
               "integer",
               "binary");

    /* read information about the block*/ 
    num_elems = array[0];
    num_vertices = array[1];
    num_per_line = array[3];   
    connectivity = new int [num_elems*num_per_line];

    if(connectivity == NULL)
      {
      vtkErrorMacro(<<"Unable to allocate memory for connectivity info");
      return;
      }

    item = num_elems*num_per_line;
    readdatablock(&geomfile,
                  "connectivity interior",
                  connectivity,
                  &item,
                  "integer",
                  "binary");

    /* insert cells */
    for(i=0;i<num_elems;i++)
      {
      nodes = new vtkIdType[num_vertices];

      //connectivity starts from 1 so node[j] will never be -ve
      for(j=0;j<num_vertices;j++)
        {
        nodes[j] = connectivity[i+num_elems*j] + firstVertexNo - 1;
        }

      /* 1 is subtracted from the connectivity info to reflect that in vtk 
         vertex  numbering start from 0 as opposed to 1 in geomfile */

      // find out element type
      switch(num_vertices) 
        {
        case 4:
          cell_type = VTK_TETRA;
          break;
        case 5:
          cell_type = VTK_PYRAMID;
          break;
        case 6:
          cell_type = VTK_WEDGE;
          break;
        case 8:
          cell_type = VTK_HEXAHEDRON;

          break;
        default:
          vtkErrorMacro(<<"Unrecognized CELL_TYPE in "<< geomFileName)
            return;
        }

      /* insert the element */
      output->InsertNextCell(cell_type,num_vertices,nodes);
      delete [] nodes;
      }
    }
  // update the firstVertexNo so that next slice/partition can be read
  firstVertexNo = firstVertexNo + num_nodes;

  // clean up
  closefile(&geomfile,"read");
  delete [] coordinates; 
  delete [] pos;
  delete [] connectivity;


}

void vtkPhastaReader::ReadFieldFile(char* fieldFileName, 
                                    int, 
                                    vtkDataSetAttributes *field, 
                                    int &noOfNodes)
{

  int i, j;
  int item;
  double *data;
  int fieldfile;

  openfile(fieldFileName,"read",&fieldfile);
  //fieldfile = fopen(FieldFileName,"rb");

  if(!fieldfile)
    {
    vtkErrorMacro(<<"Cannot open file " << FieldFileName)
      return;
    }
  int array[10], expect;

  /* read the solution */
  vtkDoubleArray* pressure = vtkDoubleArray::New();
  pressure->SetName("pressure");
  vtkDoubleArray* velocity = vtkDoubleArray::New();
  velocity->SetName("velocity");
  velocity->SetNumberOfComponents(3);
  vtkDoubleArray* temperature = vtkDoubleArray::New();
  temperature->SetName("temperature");

  expect = 3; 
  readheader(&fieldfile,"solution",array,&expect,"integer","binary");
  noOfNodes = array[0];
  this->NumberOfVariables = array[1];

  vtkDoubleArray* sArrays[4];
  for (i=0; i<4; i++)
    {
    sArrays[i] = 0;
    }
  item = noOfNodes*this->NumberOfVariables;
  data = new double[item];
  if(data == NULL)
    {
    vtkErrorMacro(<<"Unable to allocate memory for field info");
    return;
    }

  readdatablock(&fieldfile,"solution",data,&item,"double","binary");

  for (i=5; i<this->NumberOfVariables; i++)
    {
    int idx=i-5;
    sArrays[idx] = vtkDoubleArray::New();
    ostrstream aName;
    aName << "s" << idx+1 << ends;
    sArrays[idx]->SetName(aName.str());
    delete[] aName.str();
    sArrays[idx]->SetNumberOfTuples(noOfNodes);
    }

  pressure->SetNumberOfTuples(noOfNodes);
  velocity->SetNumberOfTuples(noOfNodes);
  temperature->SetNumberOfTuples(noOfNodes);
  for(i=0;i<noOfNodes;i++)
    {
    pressure->SetTuple1(i, data[i]);
    velocity->SetTuple3(i, 
                        data[noOfNodes + i], 
                        data[2*noOfNodes + i],
                        data[3*noOfNodes + i]);
    temperature->SetTuple1(i, data[4*noOfNodes + i]);
    for(j=5; j<this->NumberOfVariables; j++)
      {
      sArrays[j-5]->SetTuple1(i, data[j*noOfNodes + i]);
      }
    }

  field->AddArray(pressure);
  field->SetActiveScalars("pressure");
  pressure->Delete();
  field->AddArray(velocity);
  field->SetActiveVectors("velocity");
  velocity->Delete();
  field->AddArray(temperature);
  temperature->Delete();

  for (i=5; i<this->NumberOfVariables; i++)
    {
    int idx=i-5;
    field->AddArray(sArrays[idx]);
    sArrays[idx]->Delete();
    }

  // clean up    
  closefile(&fieldfile,"read"); 
  delete [] data;


}//closes ReadFieldFile

void vtkPhastaReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "GeometryFileName: " 
     << (this->GeometryFileName?this->GeometryFileName:"(none)")
     << endl;
  os << indent << "FieldFileName: " 
     << (this->FieldFileName?this->FieldFileName:"(none)")
     << endl;
}
