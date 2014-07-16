/*=========================================================================

   Program: ParaView
   Module:    vtkVRHMDTrackStyle.h

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
This contribution has been developed at the "Brandenburg University of
Technology Cottbus - Senftenberg" at the chair of "Media Technology."
Implemented by Stephan ROGGE
------------------------------------------------------------------------*/
// .NAME vtkVRHMDTrackStyle - a tracker style for HMD purpose
// .SECTION Description
// vtkVRHMDTrackStyle is a tracker style which inverts a given pose
// matrix and push that to a defined (compatible) property. In this way
// a user can connect the tracked head pose to the model view
// transformation matrix of a render view and use his head as a
// "navigator". This enables the usage of HMD devices with ParaView.

#ifndef __vtkVRHMDTrackStyle_h_
#define __vtkVRHMDTrackStyle_h_

#include "vtkVRInteractorStyle.h"

class vtkSMDoubleVectorProperty;
class vtkSMIntVectorProperty;
class vtkSMProxy;
class vtkSMRenderViewProxy;
class vtkTransform;

struct vtkVREventData;

class vtkVRHMDTrackStyle : public vtkVRInteractorStyle
{
public:
  static vtkVRHMDTrackStyle *New();
  vtkTypeMacro(vtkVRHMDTrackStyle, vtkVRInteractorStyle)
  void PrintSelf(ostream &os, vtkIndent indent);

  virtual int GetControlledPropertySize() { return 16; }

protected:
  vtkVRHMDTrackStyle();
  ~vtkVRHMDTrackStyle();
  virtual void HandleTracker( const vtkVREventData& data );

private:
  vtkVRHMDTrackStyle(const vtkVRHMDTrackStyle&); // Not implemented.
  void operator=(const vtkVRHMDTrackStyle&); // Not implemented.
};

#endif //__vtkVRHMDTrackStyle.h_
