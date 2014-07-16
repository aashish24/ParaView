/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOVRPostPass.h

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
// .NAME 
// .SECTION Description//
// .SECTION Implementation

// .SECTION See Also
// vtkRenderPass

#ifndef __vtkOVRPostPass_h_
#define __vtkOVRPostPass_h_

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkDualStereoPass.h"

class vtkOpenGLRenderWindow;
class vtkDepthPeelingPassLayerList; // Pimpl
class vtkShaderProgram2;
class vtkShader2;
class vtkFrameBufferObject;
class vtkTextureObject;
class vtkTimerLog;

class VTK_EXPORT vtkOVRPostPass : public vtkDualStereoPass
{
public:
  static vtkOVRPostPass *New();
  vtkTypeMacro(vtkOVRPostPass,vtkDualStereoPass);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Perform rendering according to a render state \p s.
  // \pre s_exists: s!=0
  virtual void Render(const vtkRenderState *s);
  //ETX

  // Description:
  // Release graphics resources and ask components to release their own
  // resources.
  // \pre w_exists: w!=0
  void ReleaseGraphicsResources(vtkWindow *w);

  // Description:
  vtkGetVector4Macro(DistortionK, double);
  vtkSetVector4Macro(DistortionK, double);

  // Description:
  vtkGetVector4Macro(ChromaAberration, double);
  vtkSetVector4Macro(ChromaAberration, double);

  // Description:
  vtkGetMacro(DistortionScale, double);
  void SetDistortionScale(double distScale);

protected:
  // Description:
  // Default constructor. DelegatePass is set to NULL.
  vtkOVRPostPass();

  // Description:
  // Destructor.
  virtual ~vtkOVRPostPass();

  // Description:
  void ApplyPostPass(const vtkRenderState *s, 
                        vtkTextureObject* to, 
                        int LeftEye);

  // Description:
  void DrawTextureToScreen();

  double DistortionK[4];
  double DistortionScale;
  double ChromaAberration[4];

  // Description:
  // Graphics resources.
  vtkShaderProgram2 *Program; // shader to compute lens correction
  vtkFrameBufferObject *OutputFrameBuffer;
  vtkTextureObject *OutputTexture;

  vtkTimerLog *Timer;
  int FrameCounter;

 private:
  vtkOVRPostPass(const vtkOVRPostPass&);  // Not implemented.
  void operator=(const vtkOVRPostPass&);  // Not implemented.
};

#endif // __vtkOVRPostPass_h_
