/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id: XdmfHeavyData.cxx,v 1.1 2002-12-02 17:11:03 clarke Exp $  */
/*  Date : $Date: 2002-12-02 17:11:03 $ */
/*  Version : $Revision: 1.1 $ */
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
#include "XdmfHeavyData.h"

XdmfHeavyData::XdmfHeavyData() {

  // Defaults
  this->SetDomain( "FILE" );
  this->SetFileName( "XdmfHeavyData.dod" );
  this->SetPath( "/" );
  this->SetAccess( "r" );
  this->SetNdgmHost("");
  this->SetWorkingDirectory("");

}

XdmfHeavyData::~XdmfHeavyData() {
}
