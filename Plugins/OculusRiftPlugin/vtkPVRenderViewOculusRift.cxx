/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRenderViewOculusRift.cxx

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
#include "vtkPVRenderViewOculusRift.h"

#include "vtkProcessModule.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderViewBase.h" 
#include "vtkSmartPointer.h"
#include "vtkPVView.h"
#include "vtkPVSynchronizedRenderer.h"
#include "vtkPVSynchronizedRenderWindows.h"
#include "vtkMatrix4x4.h"
#include "vtkTransform.h"

// Local includes
#include "vtkLensCorrectionPass.h" 

vtkStandardNewMacro(vtkPVRenderViewOculusRift);
//----------------------------------------------------------------------------
vtkPVRenderViewOculusRift::vtkPVRenderViewOculusRift()
{
  this->DisplaySettings[0] = 1.0;
  this->DisplaySettings[1] = 1.0;
  this->DisplaySettings[2] = -0.1;
  this->DistortionK[0] = 1.0;
  this->DistortionK[1] = 0.22;
  this->DistortionK[2] = 0.24;
  this->DistortionK[3] = 0.0; 
  this->OffscreenTextureDim[0] = 1280;
  this->OffscreenTextureDim[1] = 800;
  this->HeadOrientation = vtkTransform::New();
  this->HeadOrientation->Identity();
  this->DummyTransformation = vtkTransform::New();
  this->DummyTransformation->Identity();
  this->LensPass = NULL;
  this->TrackingClient = 1;
  this->StereoPostClient = 1;
}

//----------------------------------------------------------------------------
vtkPVRenderViewOculusRift::~vtkPVRenderViewOculusRift()
{
  this->HeadOrientation->Delete();
  this->DummyTransformation->Delete();

  if(this->LensPass)
    {
    this->LensPass->ReleaseGraphicsResources(this->GetRenderWindow());
    this->LensPass->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderViewOculusRift::Initialize(unsigned int id)
{
  this->Superclass::Initialize(id);

  this->LensPass = vtkLensCorrectionPass::New();
  this->SynchronizedRenderers->SetImageProcessingPass(this->LensPass);
}

//----------------------------------------------------------------------------
void vtkPVRenderViewOculusRift::Render(bool interactive, bool skip_rendering)
{  
  vtkCamera *camera = this->GetRenderer()->GetActiveCamera();
  bool isClient = vtkProcessModule::GetProcessType() == 
    vtkProcessModule::PROCESS_CLIENT;
  
  if(!isClient || (isClient && this->TrackingClient))
    {      
    this->DummyTransformation->DeepCopy(this->HeadOrientation); 
    camera->SetUserViewTransform(this->DummyTransformation);
    //camera->SetUseOffAxisProjection(1);
    //camera->OrthogonalizeViewUp();
    }  

 
  if(isClient)
    {
    int oldState = this->LensPass->GetEnable();
    this->LensPass->SetEnable(this->StereoPostClient);
    camera->SetUseOffAxisProjection(this->StereoPostClient);
    if(this->StereoPostClient && !oldState)
      {
      this->GetRenderer()->ResetCamera();
      this->GetRenderer()->ResetCameraClippingRange();
      camera->OrthogonalizeViewUp();
      }
    } 

  Superclass::Render(interactive, skip_rendering);        
}

//----------------------------------------------------------------------------
void vtkPVRenderViewOculusRift::SetDisplaySettings(double width, 
  double height, double distance)
{
  this->DisplaySettings[0] = width;
  this->DisplaySettings[1] = height;
  this->DisplaySettings[2] = distance;
  if(this->LensPass)
    {
    this->LensPass->SetDisplaySettings(this->DisplaySettings);
    }
  this->Modified();
}

 //----------------------------------------------------------------------------
void vtkPVRenderViewOculusRift::SetDistortionK(double distk0, double distk1, 
  double distk2, double distk3)
{
  this->DistortionK[0] = distk0;
  this->DistortionK[1] = distk1;
  this->DistortionK[2] = distk2;
  this->DistortionK[3] = distk3;
  this->Modified();
  if(this->LensPass)
    {
    this->LensPass->SetDistortionK(this->DistortionK);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderViewOculusRift::SetDistortionXCenterOffset(
  double distXCenterOffset)
{
  this->DistortionXCenterOffset = distXCenterOffset;
  this->Modified();
  if(this->LensPass)
    {
    this->LensPass->SetDistortionXCenterOffset(this->DistortionXCenterOffset);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderViewOculusRift::SetDistortionScale(double distScale)
{
  this->DistortionScale = distScale;
  this->Modified();
  if(this->LensPass)
    {
    this->LensPass->SetDistortionScale(this->DistortionScale);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderViewOculusRift::SetOffscreenTextureDim(int dimX, int dimY)
{
  this->OffscreenTextureDim[0] = dimX;
  this->OffscreenTextureDim[1] = dimY;
  this->Modified();
  if(this->LensPass)
    {
    this->LensPass->SetTextureDimension(dimX, dimY);
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderViewOculusRift::SetHeadOrientation(vtkMatrix4x4* matrix)
{
  this->HeadOrientation->SetMatrix(matrix);
  this->HeadOrientation->Modified();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPVRenderViewOculusRift::SetHeadOrientation(const double head[16])
{
  this->HeadOrientation->SetMatrix(head);
  this->HeadOrientation->Modified();
  this->Modified();    
}

//----------------------------------------------------------------------------
void vtkPVRenderViewOculusRift::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
