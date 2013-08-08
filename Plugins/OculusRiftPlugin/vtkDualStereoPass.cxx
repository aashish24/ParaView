/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDualStereoPass.cxx

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
#include "vtkDualStereoPass.h"

#include "vtkObjectFactory.h"
#include "vtkgl.h"
#include <assert.h>
#include "vtkRenderState.h"
#include "vtkRenderer.h"
#include "vtkFrameBufferObject.h"
#include "vtkTextureObject.h"
#include "vtkCamera.h"
#include "vtkTextureUnitManager.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkTimerLog.h"

vtkStandardNewMacro(vtkDualStereoPass);
//----------------------------------------------------------------------------
vtkDualStereoPass::vtkDualStereoPass()
{  
  this->Enable = 1;
  this->DisplaySettings[0] = 1.0;
  this->DisplaySettings[1] = 1.0;
  this->DisplaySettings[2] = -0.1;
  this->RenderTextureWidth = 400;
  this->RenderTextureHeight = 400;
  this->RenderTextureScaleX = 1.0;
  this->RenderTextureScaleY = 1.0;
  this->StereoFrameBuffer = NULL;
  this->TextureObjectLeft = NULL;
  this->TextureObjectRight = NULL;
  this->Timer = vtkTimerLog::New();
  this->FrameCounter = 0;

  this->Timer->LoggingOff();
  this->Timer->StartTimer();
}

//----------------------------------------------------------------------------
vtkDualStereoPass::~vtkDualStereoPass()
{
  if(this->StereoFrameBuffer)
    {
    this->StereoFrameBuffer->Delete();
    this->StereoFrameBuffer = NULL;
    }
  if(this->TextureObjectLeft)
    {
    this->TextureObjectLeft->Delete();
    this->TextureObjectLeft = NULL;
    }
  if(this->TextureObjectRight)
    {
    this->TextureObjectRight->Delete();
    this->TextureObjectRight = NULL;
    }
  this->Timer->Delete();
}

//----------------------------------------------------------------------------
void vtkDualStereoPass::SetTextureDimension(int width, int height)
{
  this->RenderTextureWidth = (float) width;
  this->RenderTextureHeight = (float) height;
}

//----------------------------------------------------------------------------
void vtkDualStereoPass::SetTextureScale(float xScale, float yScale)
{
  this->RenderTextureScaleX = xScale;
  this->RenderTextureScaleY = yScale;
}

//----------------------------------------------------------------------------
void vtkDualStereoPass::InitializeFrameBuffers(const vtkRenderState *s)
{
  vtkRenderer *r=s->GetRenderer();

  if(this->StereoFrameBuffer==0)
    {
    this->StereoFrameBuffer = vtkFrameBufferObject::New();    
    this->StereoFrameBuffer->SetContext(r->GetRenderWindow());    
    }
  if(this->TextureObjectLeft==0)
    {
    this->TextureObjectLeft = vtkTextureObject::New();
    this->TextureObjectLeft->SetContext(this->StereoFrameBuffer->GetContext());
    }   
  if(this->TextureObjectRight==0)
    {
    this->TextureObjectRight = vtkTextureObject::New();
    this->TextureObjectRight->SetContext(this->StereoFrameBuffer->GetContext());
    }   
}

//----------------------------------------------------------------------------
void vtkDualStereoPass::Render(const vtkRenderState *s)
{
  // Stop here, this pass is disabled
  if(!this->Enable)
    {
   /* GLint savedDrawBuffer;
    glGetIntegerv(GL_DRAW_BUFFER,&savedDrawBuffer);*/

    this->DelegatePass->Render(s);
      this->NumberOfRenderedProps+=
        this->DelegatePass->GetNumberOfRenderedProps();

    /*glDrawBuffer(savedDrawBuffer);*/
    return;
    }

  // Intialized FBO and texture object, when not already happend
  this->InitializeFrameBuffers(s);

 // GLint savedDrawBuffer;
 // glGetIntegerv(GL_DRAW_BUFFER,&savedDrawBuffer);

  // Render left eye to FBO with attached texture
  this->RenderToFrameBuffer(s, this->TextureObjectLeft, true);

  // Render right eye to FBO with attached texture
  this->RenderToFrameBuffer(s, this->TextureObjectRight, false);

  this->FrameCounter++;

  this->Timer->StopTimer();
  double elapsedTime = this->Timer->GetElapsedTime();
  
  if(elapsedTime >= 1.0)
    {            
    /*vtkWarningMacro(<< "Stereo Render FPS: " << 
                        (this->FrameCounter / elapsedTime));*/
    this->FrameCounter = 0;
    this->Timer->StartTimer();
    }

  //
}

// ----------------------------------------------------------------------------
void vtkDualStereoPass::RenderToFrameBuffer(const vtkRenderState *s,
  vtkTextureObject* target, bool left)
{
  this->NumberOfRenderedProps = 0;
  vtkRenderer *r=s->GetRenderer();
  vtkRenderState s2(r);  

  vtkCamera *cam = s2.GetRenderer()->GetActiveCamera();
  
  int width, height;  
  width = (int)(this->RenderTextureWidth * this->RenderTextureScaleX);
  height = (int)(this->RenderTextureHeight * this->RenderTextureScaleY);
    
  cam->SetLeftEye(left ? 1 : 0);
  cam->SetEyeSeparation(0.064);

  double x = this->DisplaySettings[0] / 2.0;
  double y = this->DisplaySettings[1] / 2.0;
  double d = this->DisplaySettings[2];

  if(left)
    {
    cam->SetScreenBottomLeft( -x, -y, -d);
    cam->SetScreenBottomRight( 0, -y, -d);
    cam->SetScreenTopRight(    0,  y, -d);
    } 
  else 
    {
    cam->SetScreenBottomLeft(  0, -y, -d);
    cam->SetScreenBottomRight( x, -y, -d);
    cam->SetScreenTopRight(    x,  y, -d);
    }

  s2.GetRenderer()->GetRenderWindow()->SetStereoRender(1);

  if(left)
    {
    s2.GetRenderer()->GetRenderWindow()->SetStereoTypeToLeft();
    }
  else
    {
    s2.GetRenderer()->GetRenderWindow()->SetStereoTypeToRight();
    }

  s2.SetFrameBuffer(this->StereoFrameBuffer);
  s2.SetPropArrayAndCount(s->GetPropArray(),s->GetPropArrayCount());

  if(target->GetWidth()!=static_cast<unsigned int>(width) ||
      target->GetHeight()!=static_cast<unsigned int>(height))
    {
    target->Create2D(width,height,4,VTK_UNSIGNED_CHAR,false);
    }  

  this->StereoFrameBuffer->SetNumberOfRenderTargets(1);  
  this->StereoFrameBuffer->SetColorBuffer(0,target);  

  // because the same FBO can be used in another pass but with several color
  // buffers, force this pass to use 1, to avoid side effects from the
  // render of the previous frame.
  this->StereoFrameBuffer->SetActiveBuffer(0);
 
  this->RenderDelegate(&s2,width,height,
             width,height,this->StereoFrameBuffer,
             target);

  this->StereoFrameBuffer->UnBind();
  
  this->NumberOfRenderedProps+=
         this->DelegatePass->GetNumberOfRenderedProps();
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkDualStereoPass::ReleaseGraphicsResources(vtkWindow *w)
{
  assert("pre: w_exists" && w!=0);
  if(this->StereoFrameBuffer!=0)
    {
    this->StereoFrameBuffer->Delete();
    this->StereoFrameBuffer = NULL;
    }
  if(this->TextureObjectLeft!=0)
    {
    this->TextureObjectLeft->Delete();
    this->TextureObjectLeft = NULL;
    }
  if(this->TextureObjectRight!=0)
    {
    this->TextureObjectRight->Delete();
    this->TextureObjectRight = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkDualStereoPass::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
