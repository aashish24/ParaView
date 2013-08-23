/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOVRCamera.cxx

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
#include "vtkOVRCamera.h"

#include "vtkObjectFactory.h"
#include "vtkgl.h"
#include <assert.h>
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkOpenGLCamera.h"
#include "vtkTransform.h"
#include "vtkHomogeneousTransform.h"
#include "vtkMath.h"

vtkStandardNewMacro(vtkOVRCamera);
//----------------------------------------------------------------------------
vtkOVRCamera::vtkOVRCamera() 
{
  this->UserViewTransform = NULL;

  this->ViewAdjust = vtkPerspectiveTransform::New();
  this->ProjectionAdjust = vtkPerspectiveTransform::New();
  this->ProjectionMatrix = vtkMatrix4x4::New();

  this->ViewAdjust->Identity();
  this->ProjectionAdjust->Identity();

  this->LeftEye = 1;
  this->ViewAngle = 60.0;
  this->ProjectionOffset[0] = 0.0;
  this->ProjectionOffset[1] = 0.0;
}

//----------------------------------------------------------------------------
vtkOVRCamera::~vtkOVRCamera() 
{
  this->ViewAdjust->Delete();
  this->ProjectionMatrix->Delete();
  this->ProjectionAdjust->Delete();
}

//----------------------------------------------------------------------------
void vtkOVRCamera::AdjustCamera()
{
  this->UseOffAxisProjection = false;
  double halfEyeSep = this->EyeSeparation * 0.5;
  double projOffsetX = this->ProjectionOffset[0];
  
  if(!this->LeftEye)
    {
    halfEyeSep = -halfEyeSep;
    projOffsetX = -projOffsetX;
    }

  this->ViewAdjust->Identity();
  this->ViewAdjust->Translate(halfEyeSep, 0.0, 0.0);

  if (!this->UserViewTransform)
    {
    this->UserViewTransform = vtkHomogeneousTransform::SafeDownCast(
                                this->ViewAdjust->MakeTransform());
    }    
  this->UserViewTransform->DeepCopy(this->ViewAdjust);  
  
  // Trigger view transformatio update (with shifted eye position)
  this->ComputeViewTransform();
  
  // Shift both projections towards the center (Paralax correaction)
  this->ProjectionAdjust->Identity();
  this->ProjectionAdjust->Translate(projOffsetX, this->ProjectionOffset[1], 0);
  
  this->Modified();
}

//----------------------------------------------------------------------------
// Return the projection transform matrix. See ComputeProjectionTransform.
vtkMatrix4x4 *vtkOVRCamera::GetProjectionTransformMatrix(double aspect,
                                                      double nearz,
                                                      double farz)
{
  this->ComputeProjectionTransform(aspect, nearz, farz);

  // return the transform
  return this->ProjectionTransform->GetMatrix();
}

//----------------------------------------------------------------------------
// Return the projection transform object. See ComputeProjectionTransform.
vtkPerspectiveTransform *vtkOVRCamera::GetProjectionTransformObject(
  double aspect, double nearz, double farz)
{
  this->ComputeProjectionTransform(aspect, nearz, farz);

  // return the transform
  return this->ProjectionTransform;
}

//----------------------------------------------------------------------------
// Return the projection transform matrix. See ComputeProjectionTransform.
vtkMatrix4x4 *vtkOVRCamera::GetCompositeProjectionTransformMatrix(
  double aspect, double nearz, double farz)
{
  // turn off stereo, the CompositeProjectionTransformMatrix is used for
  // picking, not for rendering.
  int stereo = this->Stereo;
  this->Stereo = 0;

  this->Transform->Identity();
  this->Transform->Concatenate(this->GetProjectionTransformMatrix(aspect,
                                                                  nearz,
                                                                  farz));
  this->Transform->Concatenate(this->GetViewTransformMatrix());

  this->Stereo = stereo;

  // return the transform
  return this->Transform->GetMatrix();
}

//----------------------------------------------------------------------------
void vtkOVRCamera::ComputeProjectionTransform(double aspect, double nearz, 
  double farz)
{
  nearz = 0.1;
  farz = 10000.0;

  this->ProjectionMatrix->Identity(); 

  float tanHalfFov = tan(vtkMath::RadiansFromDegrees(this->ViewAngle) * 0.5f);
  
  this->ProjectionMatrix->Element[0][0] = 1.0f / (aspect * tanHalfFov);
  this->ProjectionMatrix->Element[1][1] = 1.0f / tanHalfFov;
  this->ProjectionMatrix->Element[2][2] = farz / (nearz - farz);
  this->ProjectionMatrix->Element[3][2] = -1.0f;
  this->ProjectionMatrix->Element[2][3] = (farz * nearz) / (nearz - farz);
  this->ProjectionMatrix->Element[3][3] = 0.0f;

  this->ProjectionTransform->Identity();  

  //this->ProjectionTransform->AdjustZBuffer( -1, +1, nearz, farz );

  this->ProjectionTransform->Concatenate(this->ProjectionAdjust);
  this->ProjectionTransform->Concatenate(this->ProjectionMatrix); 

  //vtkWarningMacro(<< "Lesft? " << this->LeftEye); 
}
