/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderViewOculusRift.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------
Acknowledgement:
This contribution has been developed at the "Brandenburg University of
Technology Cottbus - Senftenberg" at the chair of "Media Technology."
Implemented by Stephan ROGGE
------------------------------------------------------------------------*/
// .NAME vtkPVOculusRiftRenderView
// .SECTION Description
// vtkPVOculusRiftRenderView is a vtkPVRenderView specialization that uses
// renders a side-by-side stereo image to screen and applies an lens
// disstortion compensation.

#ifndef __vtkPVRenderViewOculusRift_h_
#define __vtkPVRenderViewOculusRift_h_

#include "vtkPVRenderView.h"

class vtkMatrix4x4;
class vtkTransform;
class vtkPerspectiveTransform;
class vtkOVRPostPass;

class VTK_EXPORT vtkPVRenderViewOculusRift : public vtkPVRenderView
{
public:
  static vtkPVRenderViewOculusRift* New();
  vtkTypeMacro(vtkPVRenderViewOculusRift, vtkPVRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the view with an identifier. Unless noted otherwise, this method
  // must be called before calling any other methods on this class.
  // @CallOnAllProcessess
  virtual void Initialize(unsigned int id);

  // Description:
  vtkGetMacro(EyeSeparation, double);
  vtkSetMacro(EyeSeparation, double);

  // Description:
  vtkGetVector2Macro(ProjectionOffset, double);
  vtkSetVector2Macro(ProjectionOffset, double);

  // Description:
  vtkGetVector4Macro(DistortionK, double);
  vtkSetVector4Macro(DistortionK, double);

  // Description:
  vtkGetMacro(DistortionScale, double);
  vtkSetMacro(DistortionScale, double);

  // Description:
  vtkGetVector4Macro(ChromaAberation, double);
  vtkSetVector4Macro(ChromaAberation, double);

  // Description:
  vtkGetMacro(StereoPostClient, int);
  vtkSetMacro(StereoPostClient, int);
  
//BTX
protected:
  vtkPVRenderViewOculusRift();
  ~vtkPVRenderViewOculusRift();

  virtual void Render(bool interactive, bool skip_rendering);

  double EyeSeparation;
  double ProjectionOffset[2];
  double DistortionK[4];
  double DistortionScale;
  double ChromaAberation[4];
  vtkOVRPostPass *OVRPass;
  int StereoPostClient;
  
private:
  vtkPVRenderViewOculusRift(const vtkPVRenderViewOculusRift&); // Not implemented
  void operator=(const vtkPVRenderViewOculusRift&); // Not implemented
//ETX
};

#endif // __vtkPVRenderViewOculusRift_h_
