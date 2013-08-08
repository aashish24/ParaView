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

#ifndef __vtkPVRenderViewOculusRift_h
#define __vtkPVRenderViewOculusRift_h

#include "vtkPVRenderView.h"

class vtkMatrix4x4;
class vtkTransform;
class vtkLensCorrectionPass;

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
  vtkGetVector4Macro(DistortionK, double);
  void SetDistortionK(double distk0, double distk1, double distk2, double distk3);
  void SetDistortionK(double distk[4]) 
  {
    SetDistortionK(distk[0], distk[1], distk[2], distk[3]);
  };
   
  // Description:
  vtkGetMacro(DistortionXCenterOffset, double);
  void SetDistortionXCenterOffset(double distXCenterOffset);

  // Description:
  vtkGetMacro(DistortionScale, double);
  void SetDistortionScale(double distScale);

  // Description:
  vtkGetVector2Macro(OffscreenTextureDim, int);
  void SetOffscreenTextureDim(int dimX, int dimY);
  void SetOffscreenTextureDim(int dim[2])
  {
    SetOffscreenTextureDim(dim[0], dim[1]);
  }

  // Description:
  vtkGetVector3Macro(DisplaySettings, double);
  void SetDisplaySettings(double width, double height, double distance);
  void SetDisplaySettings(double settings[3])
  {
    SetDisplaySettings(settings[0], settings[1], settings[2]);
  }

  // Description:
  // Set/Get the head orientation transform matrix.
  // Default is identity.
  void SetHeadOrientation(vtkMatrix4x4* matrix);
  vtkGetObjectMacro(HeadOrientation, vtkTransform);

  // Description:
  // Set the head orientation transform matrix.
  // Default is identity.
  void SetHeadOrientation(const double head[16]);

  // Description:
  vtkGetMacro(TrackingClient, int);
  vtkSetMacro(TrackingClient, int);

  // Description:
  vtkGetMacro(StereoPostClient, int);
  vtkSetMacro(StereoPostClient, int);

//BTX
protected:
  vtkPVRenderViewOculusRift();
  ~vtkPVRenderViewOculusRift();

  virtual void Render(bool interactive, bool skip_rendering);

  int OffscreenTextureDim[2];
  double DisplaySettings[3];
  double DistortionK[4];  
  double DistortionXCenterOffset;
  double DistortionScale;
  vtkTransform *HeadOrientation;
  vtkTransform *DummyTransformation;
  vtkLensCorrectionPass *LensPass;
  int TrackingClient;
  int StereoPostClient;
  
private:
  vtkPVRenderViewOculusRift(const vtkPVRenderViewOculusRift&); // Not implemented
  void operator=(const vtkPVRenderViewOculusRift&); // Not implemented
//ETX
};

#endif
