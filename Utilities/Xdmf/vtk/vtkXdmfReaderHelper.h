/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : $Id: vtkXdmfReaderHelper.h,v 1.1 2002-12-02 17:13:54 clarke Exp $  */
/*  Date : $Date: 2002-12-02 17:13:54 $ */
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
#ifndef __vtkXdmfReaderHelper_h
#define __vtkXdmfReaderHelper_h

#include "Xdmf/Xdmf.h"


class vtkXdmfReaderHelper : public XdmfGrid
{
public :
  XdmfInt64 GetNumberOfPoints();

  vtkXdmfReaderHelper();
  ~vtkXdmfReaderHelper();

protected:
  char    *InputXMLFileName;

private :
  vtkXdmfReaderHelper( const vtkXdmfReaderHelper&); // Not implemented.
  void operator=(const vtkXdmfReaderHelper&); // Not implemented.
};

#endif // __vtkXdmfReaderHelper_h
