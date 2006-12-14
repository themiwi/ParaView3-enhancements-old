/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id: XdmfElement.h,v 1.2 2006-12-14 19:10:59 clarke Exp $  */
/*  Date : $Date: 2006-12-14 19:10:59 $ */
/*  Version : $Revision: 1.2 $ */
/*                                                                 */
/*  Author:                                                        */
/*     Jerry A. Clarke                                             */
/*     clarke@arl.army.mil                                         */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2006 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#ifndef __XdmfElement_h
#define __XdmfElement_h

#include "XdmfDOM.h"

/*!
    XdmfElement represents an Element in the LightData. For example,
    XdmfInformation, XdmfGrid, XdmfTopology etc. are all elements.
    Elements have "Attributes" which are the Name=Value pairs in the
    XML. Elements can also have children elements and CDATA which is the
    Character Data of the Element. Consider :
\verbatim

    <Element Name="Stuff">
        November 28, 1962
        <Element Name="XBounds" Value="0.0 100.0" />
    </Element>

\endverbatim
    In the first Element, Name is the one and only Attribute. The second
    Element is a child of the first and has the Attributes : Name and Value.
    November 28, 1962 is the CDATA of the first Element.
*/
class XDMF_EXPORT XdmfElement : public XdmfLightData {

public:
    XdmfElement();
    ~XdmfElement();
    XdmfConstString GetClassName() { return("XdmfElement"); } ;

    //! Set the DOM to use
    XdmfSetValueMacro(DOM, XdmfDOM *);
    //! Get the current DOM
    XdmfGetValueMacro(DOM, XdmfDOM *);
    /*! Set the XML Node
        \param Value is the low level node returned from XdmfDOM->FindElement() etc.
    */
    XdmfSetValueMacro(Element, XdmfXmlNode);

    /*! Get the XML Node
    */
    XdmfGetValueMacro(Element, XdmfXmlNode);

    //! Get the Element type : Grid, Topology, etc.
    XdmfConstString GetElementType();

    //! Initialize basic structure from XML
    XdmfInt32 UpdateInformation();

    //! Initialize all information. Possibly acessing Heavy Data.
    XdmfInt32 Update();

    //! Set the Value of an Attribute
    XdmfInt32 Set(XdmfConstString Name, XdmfConstString Value);

    //! Get the Value of An Attribute
    XdmfConstString Get(XdmfConstString Name);

protected:
    XdmfDOM     *DOM;
    XdmfXmlNode Element;
};
#endif // __XdmfElement_h
