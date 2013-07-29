/*=========================================================================

   Program: ParaView
   Module:    vtkVRHMDTrackStyle.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
/*----------------------------------------------------------------------
Acknowledgement:
This contribution is part of the EU founded project "VIT III - Virtual 
turbine engine" at the Brandenburg University of Technology Cottbus -
Senftenberg in cooperation with Rolls-Royce Deutschland. Implemented by
Stephan ROGGE
------------------------------------------------------------------------*/
#include "vtkVRHMDTrackStyle.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkVRQueue.h"
#include "vtkMatrix4x4.h"

#include <sstream>
#include <algorithm>

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVRHMDTrackStyle)

// ----------------------------------------------------------------------------
vtkVRHMDTrackStyle::vtkVRHMDTrackStyle() :
  Superclass()
{
  this->AddTrackerRole("Tracker");
}

// ----------------------------------------------------------------------------
vtkVRHMDTrackStyle::~vtkVRHMDTrackStyle()
{
}

// ----------------------------------------------------------------------------
void vtkVRHMDTrackStyle::HandleTracker( const vtkVREventData& data )
{
  vtkStdString role = this->GetTrackerRole(data.name);
  if (role == "Tracker")
    {
    if (this->ControlledProxy && this->ControlledPropertyName != NULL &&
        this->ControlledPropertyName[0] != '\0')
      {
      vtkMatrix4x4 *invTrackerMatrix = vtkMatrix4x4::New();
      invTrackerMatrix->DeepCopy(data.data.tracker.matrix);
      invTrackerMatrix->Invert();
      vtkSMPropertyHelper(this->ControlledProxy,
                          this->ControlledPropertyName).Set(
            &invTrackerMatrix->Element[0][0], 16);
      this->ControlledProxy->UpdateVTKObjects();
      }
    }
}

void vtkVRHMDTrackStyle::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
