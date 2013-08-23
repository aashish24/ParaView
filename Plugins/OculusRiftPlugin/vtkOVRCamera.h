/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDualStereoPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

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
// .NAME vtkOVRCamera - 
// .SECTION Description
//
// .SECTION Implementation

#ifndef __vtkOVRCamera_h_
#define __vtkOVRCamera_h_

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkOpenGLCamera.h"
#include "vtkPerspectiveTransform.h"

class vtkPerspectiveTransform;

class vtkOVRCamera : public vtkOpenGLCamera
{
public:
  static vtkOVRCamera *New();
  vtkTypeMacro(vtkOVRCamera,vtkOpenGLCamera);

  // Description:
  // Implement base class method.
  void AdjustCamera();

   // Description:
  // Return the projection transform matrix, which converts from camera
  // coordinates to viewport coordinates.  The 'aspect' is the
  // width/height for the viewport, and the nearz and farz are the
  // Z-buffer values that map to the near and far clipping planes.
  // The viewport coordinates of a point located inside the frustum are in the
  // range ([-1,+1],[-1,+1],[nearz,farz]).
  virtual vtkMatrix4x4 *GetProjectionTransformMatrix(double aspect,
                                                      double nearz,
                                                      double farz);

  // Description:
  // Return the projection transform matrix, which converts from camera
  // coordinates to viewport coordinates. The 'aspect' is the
  // width/height for the viewport, and the nearz and farz are the
  // Z-buffer values that map to the near and far clipping planes.
  // The viewport coordinates of a point located inside the frustum are in the
  // range ([-1,+1],[-1,+1],[nearz,farz]).
  virtual vtkPerspectiveTransform *GetProjectionTransformObject(double aspect,
                                                                double nearz,
                                                                double farz);

  // Description:
  // Return the concatenation of the ViewTransform and the
  // ProjectionTransform. This transform will convert world
  // coordinates to viewport coordinates. The 'aspect' is the
  // width/height for the viewport, and the nearz and farz are the
  // Z-buffer values that map to the near and far clipping planes.
  // The viewport coordinates of a point located inside the frustum are in the
  // range ([-1,+1],[-1,+1],[nearz,farz]).
  virtual vtkMatrix4x4 *GetCompositeProjectionTransformMatrix(double aspect,
                                                              double nearz,
                                                              double farz);

  // Description:
  vtkGetVector2Macro(ProjectionOffset, double);
  vtkSetVector2Macro(ProjectionOffset, double);

protected:
  vtkOVRCamera();
  ~vtkOVRCamera();

  void ComputeProjectionTransform(double aspect, double nearz, double farz);

  vtkPerspectiveTransform *ViewAdjust;

  double ProjectionOffset[2];
  vtkMatrix4x4 *ProjectionMatrix; 
  vtkPerspectiveTransform *ProjectionAdjust;

private:
  vtkOVRCamera(const vtkOVRCamera&);  // Not implemented.
  void operator=(const vtkOVRCamera&);  // Not implemented.
};

#endif // __vtkOVRCamera_h_