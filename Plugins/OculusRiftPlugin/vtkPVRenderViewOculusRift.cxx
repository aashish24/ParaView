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
#include "vtkSmartPointer.h"
#include "vtkRenderWindow.h"
#include "vtkPVSynchronizedRenderer.h"

// Local includes
#include "vtkOVRPostPass.h" 

vtkStandardNewMacro(vtkPVRenderViewOculusRift);
//----------------------------------------------------------------------------
vtkPVRenderViewOculusRift::vtkPVRenderViewOculusRift()
{
  this->ProjectionOffset[0] = 0.0;
  this->ProjectionOffset[1] = 0.0;
  this->DistortionK[0] = 1.0;
  this->DistortionK[1] = 0.22;
  this->DistortionK[2] = 0.24;
  this->DistortionK[3] = 0.0; 
  this->ChromaAberation[0] = 0.0;
  this->ChromaAberation[1] = 0.0;
  this->ChromaAberation[2] = 0.0;
  this->ChromaAberation[3] = 0.0; 
  this->OVRPass = NULL;
  this->StereoPostClient = 1;
}

//----------------------------------------------------------------------------
vtkPVRenderViewOculusRift::~vtkPVRenderViewOculusRift()
{
  if(this->OVRPass)
    {
    this->OVRPass->ReleaseGraphicsResources(this->GetRenderWindow());
    this->OVRPass->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkPVRenderViewOculusRift::Initialize(unsigned int id)
{
  this->Superclass::Initialize(id);

  this->OVRPass = vtkOVRPostPass::New();
  this->SynchronizedRenderers->SetImageProcessingPass(this->OVRPass);
}

//----------------------------------------------------------------------------
void vtkPVRenderViewOculusRift::Render(bool interactive, bool skip_rendering)
{  
  bool isClient = vtkProcessModule::GetProcessType() == 
    vtkProcessModule::PROCESS_CLIENT;

  if(this->OVRPass)
    {
    this->OVRPass->SetEyeSeparation(this->EyeSeparation);
    this->OVRPass->SetProjectionOffset(this->ProjectionOffset);
    this->OVRPass->SetDistortionK(this->DistortionK);
    this->OVRPass->SetDistortionScale(this->DistortionScale);
    this->OVRPass->SetChromaAberration(this->ChromaAberation); 
    }
  
  if(isClient)
    {
    int oldState = this->OVRPass->GetEnable();
    this->OVRPass->SetEnable(this->StereoPostClient);
    } 

  Superclass::Render(interactive, skip_rendering);
}

//----------------------------------------------------------------------------
void vtkPVRenderViewOculusRift::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
