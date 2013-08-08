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
// .NAME vtkDualStereoPass - 
// .SECTION Description
//
// .SECTION Implementation

#ifndef __vtkDualStereoPass_h_
#define __vtkDualStereoPass_h_

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkImageProcessingPass.h"

class vtkFrameBufferObject;
class vtkTextureObject;
class vtkTimerLog;

class vtkDualStereoPass : public vtkImageProcessingPass
{
  public:
  static vtkDualStereoPass *New();
  vtkTypeMacro(vtkDualStereoPass,vtkImageProcessingPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  vtkGetMacro(Enable, int);
  vtkSetMacro(Enable, int);

  // Description:
  vtkGetVector3Macro(DisplaySettings, double);
  vtkSetVector3Macro(DisplaySettings, double);

  //BTX
  // Description:
  // Perform rendering according to a render state \p s.
  // Call RenderOpaqueGeometry(), RenderTranslucentPolygonalGeometry(),
  // RenderVolumetricGeometry(), RenderOverlay()
  // \pre s_exists: s!=0
  virtual void Render(const vtkRenderState *s);
  //ETX

  // Description:
  // Release graphics resources and ask components to release their own
  // resources.
  // \pre w_exists: w!=0
  void ReleaseGraphicsResources(vtkWindow *w);  

  // Description:
  // Scaling the off-screen render target (e.g. to provide a kind of super sampling)
  void SetTextureDimension(int width, int height);

  // Description:
  // Scaling the off-screen render target (e.g. to provide a kind of super sampling)
  void SetTextureScale(float xScale, float yScale);
  void SetTextureScale(float scale)
    {
    SetTextureScale(scale, scale);
    }

protected:
  // Description:
  // Default constructor.
  vtkDualStereoPass();

  // Description:
  // Destructor.
  virtual ~vtkDualStereoPass();

  // Description:
  void InitializeFrameBuffers(const vtkRenderState *s);

  // Description:
  void RenderToFrameBuffer(const vtkRenderState *s,  
                            vtkTextureObject* target, 
                            bool left);
    
  int Enable;
  double DisplaySettings[3];
  float RenderTextureWidth;
  float RenderTextureHeight;
  float RenderTextureScaleX;
  float RenderTextureScaleY;
  
  vtkTextureObject *TextureObjectLeft;
  vtkTextureObject *TextureObjectRight;

  vtkTimerLog *Timer;
  int FrameCounter;

private:
  vtkDualStereoPass(const vtkDualStereoPass&);  // Not implemented.
  void operator=(const vtkDualStereoPass&);  // Not implemented.

  vtkFrameBufferObject *StereoFrameBuffer;
};

#endif // __vtkDualStereoPass_h_