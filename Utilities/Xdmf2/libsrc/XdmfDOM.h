/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id: XdmfDOM.h,v 1.2 2006-12-11 21:48:56 clarke Exp $  */
/*  Date : $Date: 2006-12-11 21:48:56 $ */
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
#ifndef __XdmfDOM_h
#define __XdmfDOM_h


#include "XdmfObject.h"

typedef XdmfPointer XdmfXmlNode;

class XdmfParameter;

//! Class for XML Parsing using the Document Object Model
/*!
This is the Base XML Parsing Object. A XdmfDOM will 
read the XML and build an internal tree structure. The
tree can then be walked and queried. Any node can be 
"serialized". This generates an XML string that implements
the node and all of its' children.

Many other Xdmf Classes (XdmfGrid, XdmfTopology, etc.) use this
class to parse and generate XML. The DOM can accept XML from a 
String or from a File. Once \b PARSED the resulting tree can be
modified by adding or deleting nodes and then "serialized" to produce
XML. For example, the following XML might be in MyFile.xml :
\verbatim
<Tag1 Name="First Parent">
  <Tag2 Name="First Child" />
  <Tag2 Name="Second Child">
    Text for Second Child
  </Tag2>
</Tag1>
\endverbatim

The DOM might manipulate the XML with :

\code
  XdmfDOM    *DOM = new XdmfDOM();
  XdmfXmlNode  *Parent, *FirstChild, *SecondChild;

  // Parse the XML File
  DOM->SetInputFileName("MyFile.xml");
  DOM->Parse();
  // Find the first element with TAG = Tag1
  Parent = DOM->FindElement("Tag1");
  // Find the first (zero based) Tag2 below Parent
  FirstChild = DOM->FindElement("Tag2", 0, Parent);
  cout << "The Name of the First Child is <" << DOM->Get(FirstChild, "Name") << ">" << endl;
  // Find the second (zero based) Tag2 below Parent
  SecondChild = DOM->FindElement("Tag2", 1, Parent);
  DOM->Set(SecondChild, "Age", "10");
  DOM->DeleteNode(FirstChild);
  cout << endl << "XML = " << endl << DOM->Serialize(Parent) << endl;
\endcode

Would Procude the following Output:
\verbatim
The Name of the First Child is <First Child>

XML =
<Tag1 Name="First Parent">
  <Tag2 Name="Second Child" Age="10">
    Text for Second Child
  </Tag2>
</Tag1>
\endverbatim

*/
class XDMF_EXPORT XdmfDOM : public XdmfObject {

public :

  XdmfDOM();
  ~XdmfDOM();

  XdmfConstString GetClassName() { return("XdmfDOM"); } ;

  //! Set the FileName of the XML Description : stdin or Filename
        XdmfInt32 SetInputFileName( XdmfConstString Filename );

  //! Set the FileName of the XML Description : stderr or Filename
        XdmfInt32 SetOutputFileName( XdmfConstString Filename );

  //! Get the FileName of the XML Description
         XdmfGetStringMacro( InputFileName );

  //! Get the FileName of the XML Description
         XdmfGetStringMacro( OutputFileName );

  //! Get the XML destination
        XdmfGetValueMacro( Output, ostream *);
  
  //! Set the XML destination
        XdmfSetValueMacro( Output, ostream *);

  //! Get the XML destination
        XdmfGetValueMacro( Input, istream *);
  
  //! Set the XML destination
        XdmfSetValueMacro( Input, istream *);

//! Generate a Standard XDMF Header
  XdmfInt32 GenerateHead( void );
//! Output a String to the XML document
  XdmfInt32 Puts( XdmfConstString String );
//! Generate a Standard XDMF Tail i.e. </Xdmf>
  XdmfInt32 GenerateTail( void );

//! Get the rest of the documant as a string
  XdmfConstString Gets( void );

//! Return the Low Level root of the tree
  XdmfXmlNode GetTree( void ) {return(this->tree);} ;

  //! Parse XML without re-initializing entire DOM
  XdmfXmlNode __Parse( XdmfConstString xml );

  //! Re-Initialize and Parse 
  XdmfInt32 Parse(XdmfConstString xml = NULL );


//! Get the Root Node
  XdmfXmlNode GetRoot( void );

  //! Get the Number of immediate Children
  XdmfInt64 GetNumberOfChildren( XdmfXmlNode node = NULL);
  //! Get The N'th Child
  XdmfXmlNode GetChild( XdmfInt64 Index , XdmfXmlNode Node );
  //! Get Number of Attribute in a Node
  XdmfInt32 GetNumberOfAttributes( XdmfXmlNode Node );
  //! Get Attribute Name by Index
  XdmfConstString GetAttribute( XdmfXmlNode Node, XdmfInt32 Index );
  //! Is the XdmfXmlNode a child of "Start" in this DOM
  XdmfInt32  IsChild( XdmfXmlNode ChildToCheck, XdmfXmlNode Start = NULL );
  //! Convert DOM to XML String
  XdmfConstString Serialize(XdmfXmlNode node = NULL);
  //! Insert a node into a DOM
  XdmfInt32 Insert(XdmfXmlNode parent, XdmfXmlNode node);
  //! Create a node from an XML string and insert it in the DOM
  XdmfInt32 InsertFromString(XdmfXmlNode parent, XdmfConstString xml );
  //! Delete a node
  XdmfInt32 DeleteNode(XdmfXmlNode node);
  //! Find the n'th occurance of a certain node type
/*!
Walk the tree and find the first
element that is of a certain type. 
Index ( 0 based ) can be used to find the n'th
node that satisfies the criteria. The search can also
tree starting at a particular node.
*/
  XdmfXmlNode FindElement(XdmfConstString TagName,
      XdmfInt32 Index= 0,
      XdmfXmlNode Node = NULL );
//! Find the Node that has Attribute="Value"
  XdmfXmlNode FindElementByAttribute(XdmfConstString Attribute,
      XdmfConstString Value,
      XdmfInt32 Index= 0,
      XdmfXmlNode Node = NULL );
  //! Find the number of nodes of a certain type
  XdmfInt32 FindNumberOfElements(XdmfConstString TagName,
      XdmfXmlNode Node = NULL );
//! Find the number if Nodes that has Attribute="Value"
  XdmfInt32 FindNumberOfElementsByAttribute(XdmfConstString Attribute,
      XdmfConstString Value,
      XdmfXmlNode Node = NULL );

//! Get the default NDGM Host to use for HDF5 files
  XdmfGetStringMacro( NdgmHost );
//! Set the default NDGM Host to use for HDF5 files
  XdmfSetStringMacro( NdgmHost );

//! Get the default Working directory to use for HDF5 files
  XdmfGetStringMacro( WorkingDirectory );
//! Set the default Working directory to use for HDF5 files
  XdmfSetStringMacro( WorkingDirectory );

//! Get the Value of an Attribute from an Element
/*!
Get the various attributes from a node. If the XML is :
\verbatim
  <Tag Name="Test" Type="Data">
  file.h5
  </Tag>
\endverbatim
\code
Dom->Get(Node, "Name")  // will return "Test"
Dom->Get(Node, "Type")  // will return "Data"
Dom->Get(Node, "Other")  // will return NULL ; there is none
Dom->Get(Node, "CData")  // will return "file.h5" ; the Character Data
\endcode

*/
  XdmfConstString  Get( XdmfXmlNode Node, XdmfConstString Attribute );

//! Set a new Attribute=Value in a Node
  void    Set( XdmfXmlNode Node, XdmfConstString Attribute, XdmfConstString Value );

protected :

XdmfString      NdgmHost;
XdmfString      WorkingDirectory;
XdmfString      InputFileName;
XdmfString      OutputFileName;
ostream         *Output;
istream         *Input;
XdmfPointer     Doc;
XdmfString      xml;
XdmfXmlNode     tree;
XdmfString      LastDOMGet;

};

extern XDMF_EXPORT XdmfDOM *HandleToXdmfDOM( XdmfConstString Source );
#endif
