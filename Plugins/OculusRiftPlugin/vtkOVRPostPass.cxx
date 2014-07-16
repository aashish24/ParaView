/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOVRPostPass.cxx

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
#include "vtkOVRPostPass.h"

#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkgl.h"
#include <assert.h>
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkRenderState.h"
#include "vtkFrameBufferObject.h"
#include "vtkTextureObject.h"
#include "vtkShaderProgram2.h"
#include "vtkShader2.h"
#include "vtkShader2Collection.h"
#include "vtkUniformVariables.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkOpenGLError.h"
#include "vtkTextureUnitManager.h"
#include "vtkTimerLog.h"

vtkStandardNewMacro(vtkOVRPostPass);

extern const char *vtkOVRPostShaderSimple_fs;
extern const char *vtkOVRPostShaderFull_fs;

// ----------------------------------------------------------------------------
vtkOVRPostPass::vtkOVRPostPass()
{
  this->Enable = 1;
  this->DistortionK[0] = 1.0;
  this->DistortionK[1] = 0.22;
  this->DistortionK[2] = 0.24;
  this->DistortionK[3] = 0.0;
  this->DistortionScale = 1.0;
  this->ChromaAberration[0] = 0.0;
  this->ChromaAberration[1] = 0.0;
  this->ChromaAberration[2] = 0.0;
  this->ChromaAberration[3] = 0.0; 
  this->Program = 0;
  this->OutputFrameBuffer = 0;
  this->OutputTexture = 0;
  this->Timer = vtkTimerLog::New();
  this->FrameCounter = 0;

  this->Timer->LoggingOff();
  this->Timer->StartTimer();
}

// ----------------------------------------------------------------------------
vtkOVRPostPass::~vtkOVRPostPass()
{
  if(this->Program!=0)
    {
    this->Program->Delete();
    this->OutputTexture = 0;
    }
  if(this->OutputFrameBuffer!=0)
    {
    this->OutputFrameBuffer->Delete();
    this->OutputFrameBuffer = 0;
    }
  if(this->OutputTexture!=0)
    {
    this->OutputTexture->Delete();
    this->OutputTexture = 0;
    }
}

// ----------------------------------------------------------------------------
void vtkOVRPostPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

// ----------------------------------------------------------------------------
void vtkOVRPostPass::SetDistortionScale(double distScale)
{
  this->DistortionScale = distScale;
  this->SetRenderTextureScale(distScale, distScale);
}
// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void vtkOVRPostPass::Render(const vtkRenderState *s)
{
  assert("pre: s_exists" && s!=0);

  this->NumberOfRenderedProps=0;  

  GLint savedDrawBuffer;
  glGetIntegerv(GL_DRAW_BUFFER,&savedDrawBuffer);

  // Stop here, this pass is disabled
  if(!this->Enable)
    {
    glDrawBuffer(savedDrawBuffer);
    this->DelegatePass->Render(s);
      this->NumberOfRenderedProps+=
        this->DelegatePass->GetNumberOfRenderedProps();
    return;
    }

  if(this->DelegatePass!=0)
    {
    vtkRenderer *r=s->GetRenderer();
    // Test for Hardware support. If not supported, just render the delegate.
    bool supported=vtkFrameBufferObject::IsSupported(r->GetRenderWindow());

    vtkOpenGLRenderWindow *renwin
      = vtkOpenGLRenderWindow::SafeDownCast(r->GetRenderWindow());
    vtkOpenGLExtensionManager *extensions = renwin->GetExtensionManager();

    extensions->LoadExtension("GL_ARB_framebuffer_object");

    if(!supported)
      {
      vtkErrorMacro("FBOs are not supported by the context. Cannot detect"
        "edges on the image.");
      }
    if(supported)
      {
      supported=vtkTextureObject::IsSupported(r->GetRenderWindow());
      if(!supported)
        {
        vtkErrorMacro("Texture Objects are not supported by the context. "
          "Cannot detect edges on the image.");
        }
      }
    if(supported)
      {
      supported=
        vtkShaderProgram2::IsSupported(static_cast<vtkOpenGLRenderWindow *>(
                                        r->GetRenderWindow()));
      if(!supported)
        {
        vtkErrorMacro("GLSL is not supported by the context. "
          "Cannot detect edges on the image.");
        }
      }
    if(!supported)
      {
      this->DelegatePass->Render(s);
      this->NumberOfRenderedProps+=
        this->DelegatePass->GetNumberOfRenderedProps();
      return;
      }
    
    // Render the entire scene to FBO with attached TextureObjectLeft and 
    // TextureObjectRight
    Superclass::Render(s);
    
    // Apply lens correction fragment shader for the left texture
    this->ApplyPostPass(s, this->TextureObjectLeft, 1);

    // Apply lens correction fragment shader for the right texture
    this->ApplyPostPass(s, this->TextureObjectRight, 0);   

    // Finally, draw the Output texture to screen (Full screen quad shader)
    this->DrawTextureToScreen();

    //glDrawBuffer(savedDrawBuffer);
    
    this->FrameCounter++;

    this->Timer->StopTimer();
    double elapsedTime = this->Timer->GetElapsedTime();
  
    if(elapsedTime >= 1.0)
      {            
      /*vtkWarningMacro(<< "Complete FPS: " << 
                          (this->FrameCounter / elapsedTime));*/
      this->FrameCounter = 0;
      this->Timer->StartTimer();
      }
    }
  else
    {
    vtkWarningMacro(<<" no delegate.");
    }

  vtkOpenGLCheckErrorMacro("failed after Render");
}

// ----------------------------------------------------------------------------
void vtkOVRPostPass::ApplyPostPass(const vtkRenderState *s, 
  vtkTextureObject* to, int LeftEye)
{
  vtkRenderer *r = s->GetRenderer();

  int toWidth = to->GetWidth();
  int toHeight = to->GetHeight(); 
  
  int size[2];
  s->GetWindowSize(size);
  int width = size[0];
  int height = size[1];
  int halfwidth = width / 2;
  int XOffset = LeftEye ? 0 : halfwidth;
  
  if(!this->OutputFrameBuffer)
  {
    this->OutputFrameBuffer = vtkFrameBufferObject::New();
    this->OutputFrameBuffer->SetContext(r->GetRenderWindow());
  }
  if(!this->OutputTexture)
    {
    this->OutputTexture = vtkTextureObject::New();
    this->OutputTexture->SetContext(this->OutputFrameBuffer->GetContext());
    }
  if(this->OutputTexture->GetWidth()!=static_cast<unsigned int>(width) ||
      this->OutputTexture->GetHeight()!=static_cast<unsigned int>(height))
    {
    this->OutputTexture->Create2D(width,height,4,VTK_UNSIGNED_CHAR,false);
    }  

  this->OutputFrameBuffer->SetNumberOfRenderTargets(1);  
  this->OutputFrameBuffer->SetColorBuffer(0,this->OutputTexture);  

  // because the same FBO can be used in another pass but with several color
  // buffers, force this pass to use 1, to avoid side effects from the
  // render of the previous frame.
  this->OutputFrameBuffer->SetActiveBuffer(0);   

  if(this->Program==0)
    {
    this->Program=vtkShaderProgram2::New();
    this->Program->SetContext(
      static_cast<vtkOpenGLRenderWindow *>(
      this->OutputFrameBuffer->GetContext()));
    vtkShader2 *fsShader=vtkShader2::New();
    fsShader->SetType(VTK_SHADER_TYPE_FRAGMENT);
    fsShader->SetSourceCode(vtkOVRPostShaderFull_fs);
    fsShader->SetContext(this->Program->GetContext());
    this->Program->GetShaders()->AddItem(fsShader);
    fsShader->Delete();
    }

  this->Program->Build();

  if(this->Program->GetLastBuildStatus()
      !=VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    vtkErrorMacro("Couldn't build the shader program. At this point , it can be an error in a shader or a driver bug.");

    // restore some state.
    vtkgl::ActiveTexture(vtkgl::TEXTURE0);
    return;
    }

  vtkUniformVariables *var=this->Program->GetUniformVariables();
  vtkTextureUnitManager *tu=
    static_cast<vtkOpenGLRenderWindow *>(r->GetRenderWindow())->GetTextureUnitManager();
    
  // the actual to as opengl sources
  int id0=tu->Allocate();

  vtkgl::ActiveTexture(vtkgl::TEXTURE0+id0);
  to->Bind();
  /*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);*/
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
 // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8);

  var=this->Program->GetUniformVariables();

  float DistXOffset = this->ProjectionOffset[0];
  
  if (!LeftEye) 
    {      
    DistXOffset = -DistXOffset;
    }

  float wvp = halfwidth / float(width),
        hvp = height / float(height),
        x = float(XOffset) / float(width),
        y = float(0) / float(height);

  float as = float(halfwidth) / float(height);

  // Note: Due to shader excution which results in drawing a distorted image to the 
  // left or right half of the frame buffer, the following parameter needs to be
  // defined in texture corrdinates [0 - 1]. 
  float lensCenter[2] = { (wvp + DistXOffset*0.5), y + hvp*0.5f};
  float screenCenter[2] = { wvp, y + hvp*0.5f};

  // MA: This is more correct but we would need higher-res texture vertically; 
  // we should adopt this once we have asymmetric input texture scale.
  float scaleFactor = 1.0f / this->DistortionScale; 
  float scale[2] = {(wvp/1) * scaleFactor, (hvp/2) * scaleFactor * as};
  float scaleIn[2] = {1.0/wvp, (2/hvp) / as};
  float distortionK[4] = {(float) this->DistortionK[0], 
                          (float) this->DistortionK[1], 
                          (float) this->DistortionK[2], 
                          (float) this->DistortionK[3]}; 
  float chromaAberr[4] = {(float) this->ChromaAberration[0], 
                          (float) this->ChromaAberration[1], 
                          (float) this->ChromaAberration[2], 
                          (float) this->ChromaAberration[3]};   
  
  //float lensCenter[2] = {xvp + (wvp + DistortionXCenterOffset * 0.5f)*0.5f, yvp + hvp*0.5f};
  //float screenCenter[2] = {xvp + wvp*0.5f, yvp + hvp*0.5f};
  //float scale[2] = {(wvp/2.0f) * scaleFactor, (hvp/2.0f) * scaleFactor * as};
  //float scaleIn[2] = {(2.0f/wvp), (2.0f/hvp) / as};

  var->SetUniformi("source",1,&id0);
  var->SetUniformf("LensCenter",2,lensCenter);
  var->SetUniformf("ScreenCenter",2,screenCenter);
  var->SetUniformf("Scale",2,scale);
  var->SetUniformf("ScaleIn",2,scaleIn);
  var->SetUniformf("HmdWarpParam",4,distortionK);
  var->SetUniformf("ChromAbParam",4,chromaAberr);

  this->Program->Use();

  if(!this->Program->IsValid())
    {
    vtkErrorMacro(<<this->Program->GetLastValidateLog());
    }    

  //Draw an orthogonal full screen quad for post-process the two rendered images
  this->OutputFrameBuffer->Start(width, height, false);  
  if(LeftEye)
    {
    this->OutputFrameBuffer->RenderQuad(0, halfwidth - 1, 0, height - 1);
    } 
  else
    {
    this->OutputFrameBuffer->RenderQuad(halfwidth, width - 1, 0, height - 1);
    }

  this->OutputFrameBuffer->UnBind();
  
  this->Program->Restore();

  to->UnBind();
  vtkgl::ActiveTexture(vtkgl::TEXTURE0+id0);
  tu->Free(id0);
}

// ----------------------------------------------------------------------------
// Description:
void vtkOVRPostPass::DrawTextureToScreen()
{
  int w = this->OutputTexture->GetWidth();
  int h = this->OutputTexture->GetHeight();

  //int outputId = tu->Allocate();
  

  GLint db;
  

    // after mucking about with FBO bindings be sure
    // we're saving the default fbo attributes/blend function
    vtkgl::BindFramebuffer(vtkgl::FRAMEBUFFER, 0);
    glPushAttrib(GL_COLOR_BUFFER_BIT);
    vtkOpenGLCheckErrorMacro("failed at glPushAttrib");

 /*glPushAttrib(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT|GL_LIGHTING); 
 glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);

  // per-fragment operations
  glDisable(GL_ALPHA_TEST);
  glDisable(GL_STENCIL_TEST);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glDisable(GL_INDEX_LOGIC_OP);
  glDisable(GL_COLOR_LOGIC_OP);

  // framebuffers have their color premultiplied by alpha.
  vtkgl::BlendFuncSeparate(GL_ONE,GL_ONE_MINUS_SRC_ALPHA,
                            GL_ONE,GL_ONE_MINUS_SRC_ALPHA);

  // fixed vertex shader
  glDisable(GL_LIGHTING);

  // fixed fragment shader
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_FOG);

  glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);// client to server

  vtkgl::ActiveTexture(vtkgl::TEXTURE0);*/
  /*this->OutputTexture->Bind();  
  this->OutputTexture->CopyToFrameBuffer(0, 0, w-1, h-1, 0, 0, w, h);
  this->OutputTexture->UnBind();
   */
 
    // draw to fbo
  this->OutputFrameBuffer->SetNumberOfRenderTargets(1);
  this->OutputFrameBuffer->SetColorBuffer(0,this->OutputTexture);  

  // because the same FBO can be used in another pass but with several color
  // buffers, force this pass to use 1, to avoid side effects from the
  // render of the previous frame.
  this->OutputFrameBuffer->SetActiveBuffer(0);   

  this->OutputFrameBuffer->Bind();

  vtkOpenGLCheckErrorMacro("failed at vtkgl::BindFramebuffer");

  GLint framebufferBinding;
  glGetIntegerv(vtkgl::FRAMEBUFFER_BINDING_EXT,&framebufferBinding);

  this->OutputFrameBuffer->UnBind();

  // read from default
  vtkgl::BindFramebuffer(vtkgl::FRAMEBUFFER, framebufferBinding);

  glGetIntegerv(GL_DRAW_BUFFER, &db);
  glReadBuffer(db);

  GLenum status = vtkgl::CheckFramebufferStatus(vtkgl::READ_FRAMEBUFFER);
    if (status!=vtkgl::FRAMEBUFFER_COMPLETE)
      {
      vtkErrorMacro("FBO is incomplete " << status);
      }

   // read from default
    vtkgl::BindFramebuffer(vtkgl::READ_FRAMEBUFFER, framebufferBinding);

    vtkOpenGLCheckErrorMacro("failed at vtkgl::BindFramebuffer");

     // read from default
    vtkgl::BindFramebuffer(vtkgl::DRAW_FRAMEBUFFER, 0);

    vtkgl::BlitFramebuffer(0, 0,
                           w, h,
                           0, 0,
                           w, h,
                           GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
                           GL_NEAREST);

    vtkOpenGLCheckErrorMacro("failed at glBlitFramebuffer");
     
     // restore default fbo for both read+draw
    vtkgl::BindFramebuffer(vtkgl::FRAMEBUFFER, 0);

  glPopAttrib();
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkOVRPostPass::ReleaseGraphicsResources(vtkWindow *w)
{
  assert("pre: w_exists" && w!=0);

  this->Superclass::ReleaseGraphicsResources(w);

  if(this->Program!=0)
    {
    this->Program->ReleaseGraphicsResources();
    }
  if(this->OutputFrameBuffer!=0)
    {
    this->OutputFrameBuffer->Delete();
    this->OutputFrameBuffer = 0;
    }
  if(this->OutputTexture!=0)
    {
    this->OutputTexture->Delete();
    this->OutputTexture = 0;
    }
}
