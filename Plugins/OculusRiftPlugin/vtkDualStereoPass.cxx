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
#include "vtkOVRCamera.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkTimerLog.h"
#include "vtkMath.h"

//----------------------------------------------------------------------------
class vtkMyFBO : public vtkFrameBufferObject
{
public:
  static vtkMyFBO* New();
  vtkTypeMacro(vtkMyFBO,vtkFrameBufferObject);
  void SetLastSize(int size[2])
  {
    this->LastSize[0] = size[0];
    this->LastSize[1] = size[1];
  }
protected:
  vtkMyFBO() {}
  ~vtkMyFBO() {}
private:
  vtkMyFBO(const vtkMyFBO&);  // Not implemented.
  void operator=(const vtkMyFBO&);  // Not implemented.
};

vtkStandardNewMacro(vtkMyFBO);

//----------------------------------------------------------------------------
class vtkDualStereoPass::vtkInternal
{
public:
  GLuint MultiSampleFBO;
  GLuint FBOColorRB;
  GLuint FBODepthRB;

  GLuint StereoResolveFBO;

  int FBOSamples;
};

vtkStandardNewMacro(vtkDualStereoPass);
//----------------------------------------------------------------------------
vtkDualStereoPass::vtkDualStereoPass()
{  
  this->Enable = 1;  
  this->ProjectionOffset[0] = 0.0;
  this->ProjectionOffset[1] = 0.0;
  this->RenderTextureScale[0] = 1.0;
  this->RenderTextureScale[1] = 1.0;
  this->TextureObjectLeft = NULL;
  this->TextureObjectRight = NULL;
  this->Timer = vtkTimerLog::New();
  this->FrameCounter = 0;

  this->Timer->LoggingOff();
  this->Timer->StartTimer();

  this->OVRCamera = vtkOVRCamera::New();

  this->MultiSamples = 4;

  this->Internals = new vtkInternal();

  this->LastFBOWidth = 0;
  this->LastFBOHeight = 0;
}

//----------------------------------------------------------------------------
vtkDualStereoPass::~vtkDualStereoPass()
{
  if(this->Internals->FBOColorRB)
    {
    vtkgl::FramebufferRenderbufferEXT(vtkgl::FRAMEBUFFER_EXT, 
                                      vtkgl::COLOR_ATTACHMENT0_EXT, 
                                      vtkgl::RENDERBUFFER_EXT, 
                                      0);
    vtkgl::DeleteRenderbuffersEXT(1, &this->Internals->FBOColorRB);
    }
  if(this->Internals->FBODepthRB)
    {
    vtkgl::FramebufferRenderbufferEXT(vtkgl::FRAMEBUFFER_EXT, 
                                       vtkgl::DEPTH_ATTACHMENT_EXT, 
                                       vtkgl::RENDERBUFFER_EXT, 
                                       0);
    vtkgl::DeleteRenderbuffersEXT(1, &this->Internals->FBODepthRB);
    }  
  if(this->Internals->StereoResolveFBO)
    {
    vtkgl::DeleteFramebuffersEXT(1, &this->Internals->StereoResolveFBO);
    }
  if(this->Internals->MultiSampleFBO)
    {
    vtkgl::DeleteFramebuffersEXT(1, &this->Internals->MultiSampleFBO);
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
  this->OVRCamera->Delete();

  delete this->Internals;

}

//----------------------------------------------------------------------------
void vtkDualStereoPass::InitializeFrameBuffers(const vtkRenderState *s, 
  int fboWidth, int fboHeight)
{
  vtkRenderer *r=s->GetRenderer();

  vtkOpenGLRenderWindow *context
      = dynamic_cast<vtkOpenGLRenderWindow*>(r->GetRenderWindow());

  vtkOpenGLExtensionManager* mgr = context->GetExtensionManager();

  mgr->LoadExtension("GL_ARB_framebuffer_object");
  mgr->LoadExtension("GL_EXT_framebuffer_object");
  mgr->LoadExtension("GL_EXT_texture_object");
  mgr->LoadExtension("GL_ARB_texture_multisample");    
  mgr->LoadExtension("GL_EXT_framebuffer_multisample");
  mgr->LoadExtension("GL_EXT_framebuffer_blit");
  mgr->LoadExtension("GL_ARB_texture_non_power_of_two");

  if(this->TextureObjectLeft==0)
    {
    this->TextureObjectLeft = vtkTextureObject::New();
    this->TextureObjectLeft->SetContext(r->GetRenderWindow());
    }   
  if(this->TextureObjectRight==0)
    {
    this->TextureObjectRight = vtkTextureObject::New();
    this->TextureObjectRight->SetContext(r->GetRenderWindow());
    }   

  if(!this->Internals->StereoResolveFBO)
    {
    vtkgl::GenFramebuffersEXT(1, &this->Internals->StereoResolveFBO);
    }
  if(!this->Internals->MultiSampleFBO)
    {
    vtkgl::GenFramebuffersEXT(1, &this->Internals->MultiSampleFBO);
    }
  
  if(TextureObjectLeft->GetWidth()!=static_cast<unsigned int>(fboWidth) ||
       TextureObjectLeft->GetHeight()!=static_cast<unsigned int>(fboHeight))
    {
    this->TextureObjectLeft->Create2D(fboWidth,fboHeight,4,VTK_UNSIGNED_CHAR,false);
    this->TextureObjectLeft->SetWrapS(vtkTextureObject::Clamp);
    this->TextureObjectLeft->SetWrapT(vtkTextureObject::Clamp);
    this->TextureObjectLeft->SetLinearMagnification(true);
    this->TextureObjectLeft->SetMinificationFilter(vtkTextureObject::Linear);    
    }
  if(TextureObjectRight->GetWidth()!=static_cast<unsigned int>(fboWidth) ||
       TextureObjectRight->GetHeight()!=static_cast<unsigned int>(fboHeight))
    {       
    this->TextureObjectRight->Create2D(fboWidth,fboHeight,4,VTK_UNSIGNED_CHAR,false);
    this->TextureObjectRight->SetWrapS(vtkTextureObject::Clamp);
    this->TextureObjectRight->SetWrapT(vtkTextureObject::Clamp);
    this->TextureObjectRight->SetLinearMagnification(true);
    this->TextureObjectRight->SetMinificationFilter(vtkTextureObject::Linear);     
    }

  if(this->LastFBOWidth == 0 || this->LastFBOWidth != fboWidth || 
    this->LastFBOHeight == 0 || this->LastFBOHeight != fboHeight)
    {
    // Create the multisample color render buffer image and attach it to the
    // second FBO.
    vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT, this->Internals->MultiSampleFBO); 

    if(this->Internals->FBOColorRB)
      {
      vtkgl::FramebufferRenderbufferEXT(vtkgl::FRAMEBUFFER_EXT, vtkgl::COLOR_ATTACHMENT0_EXT, vtkgl::RENDERBUFFER_EXT, 0);
      vtkgl::DeleteRenderbuffersEXT(1, &this->Internals->FBOColorRB);
      }
    if(this->Internals->FBODepthRB)
      {
      vtkgl::FramebufferRenderbufferEXT(vtkgl::FRAMEBUFFER_EXT, vtkgl::DEPTH_ATTACHMENT_EXT, vtkgl::RENDERBUFFER_EXT, 0);
      vtkgl::DeleteRenderbuffersEXT(1, &this->Internals->FBODepthRB);
      }     

    vtkgl::GenRenderbuffersEXT(1, &this->Internals->FBOColorRB);
    vtkgl::BindRenderbufferEXT(vtkgl::RENDERBUFFER_EXT, this->Internals->FBOColorRB);
    vtkgl::RenderbufferStorageMultisampleEXT(vtkgl::RENDERBUFFER_EXT, this->MultiSamples, GL_RGBA8, fboWidth, fboHeight);
    vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT, vtkgl::RENDERBUFFER_SAMPLES_EXT, &this->Internals->FBOSamples);
    vtkgl::FramebufferRenderbufferEXT(vtkgl::FRAMEBUFFER_EXT, vtkgl::COLOR_ATTACHMENT0_EXT, vtkgl::RENDERBUFFER_EXT, this->Internals->FBOColorRB);

    // Create the multisample depth render buffer image and attach it to the
    // second FBO.
    this->CheckFramebufferStatus();
           
    vtkgl::GenRenderbuffersEXT(1, &this->Internals->FBODepthRB);
    vtkgl::BindRenderbufferEXT(vtkgl::RENDERBUFFER_EXT, this->Internals->FBODepthRB);
    vtkgl::RenderbufferStorageMultisampleEXT(vtkgl::RENDERBUFFER_EXT, this->MultiSamples, GL_DEPTH_COMPONENT, fboWidth, fboHeight);
    vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT, vtkgl::RENDERBUFFER_SAMPLES_EXT, &this->Internals->FBOSamples);
    vtkgl::FramebufferRenderbufferEXT(vtkgl::FRAMEBUFFER_EXT, vtkgl::DEPTH_ATTACHMENT_EXT, vtkgl::RENDERBUFFER_EXT, this->Internals->FBODepthRB);

    // Verify that the second FBO is 'complete'. Once the FBO is complete it is
    // ready for us to render to.
    this->CheckFramebufferStatus();

    // Unbind the FBO and switch back to normal window rendering.
    vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT, 0);

    this->LastFBOWidth = fboWidth;
    this->LastFBOHeight = fboHeight;
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

  vtkRenderer *r=s->GetRenderer();  
  int *size = r->GetRenderWindow()->GetSize();
  int vpWidth, vpHeight, fboWidth, fboHeight;  

  vpWidth = (size[0] / 2.0);
  vpHeight = size[1];    
  fboWidth = vpWidth * this->RenderTextureScale[0];
  fboHeight = vpHeight * this->RenderTextureScale[1];

  // Intialized FBO and texture object, when not already happend
  this->InitializeFrameBuffers(s, fboWidth, fboHeight);
  
  vtkRenderState s2(r);  
  vtkCamera *cam = s2.GetRenderer()->GetActiveCamera();
  cam->Register(this);

  if(cam != this->OVRCamera)
    {
    this->OVRCamera->DeepCopy(cam);
    }  

  // Adjust the camera parameters to the right eye
  this->OVRCamera->SetLeftEye(1);
  this->OVRCamera->SetEyeSeparation(this->EyeSeparation);
  this->OVRCamera->SetProjectionOffset(this->ProjectionOffset);
  this->OVRCamera->AdjustCamera(); 

  s2.GetRenderer()->SetActiveCamera(this->OVRCamera);

  this->NumberOfRenderedProps = 0;

  // Render left eye to FBO with attached texture
  this->RenderMS(&s2,
              fboWidth,fboHeight,
              0, 0,
              vpWidth, vpHeight, 
              this->TextureObjectLeft);
    
  // Adjust the camera parameters to the right eye
  this->OVRCamera->SetLeftEye(0);
  this->OVRCamera->AdjustCamera(); 

  // Render right eye to FBO with attached texture
  this->RenderMS(&s2,
              fboWidth,fboHeight,
              vpWidth, 0,
              vpWidth, vpHeight, 
              this->TextureObjectRight);
  
  this->NumberOfRenderedProps+=
    this->DelegatePass->GetNumberOfRenderedProps();

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

/*
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glPushAttrib(GL_CURRENT_BIT);

  glDisable(GL_LIGHTING);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

  glViewport(0, 0, vpWidth, vpHeight);  
    
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, this->TextureObjectLeft->GetHandle());

  glBegin(GL_QUADS);
      glTexCoord2f(0.0f, 0.0f);
      glVertex3f(-1.0f, -1.0f, -1.0f);

      glTexCoord2f(1.0f, 0.0f);
      glVertex3f(1.0f, -1.0f, -1.0f);

      glTexCoord2f(1.0f, 1.0f);
      glVertex3f(1.0f, 1.0f, -1.0f);

      glTexCoord2f(0, 1.0f);
      glVertex3f(-1.0f, 1.0f, -1.0f);
  glEnd();

  glViewport(vpWidth, 0, vpWidth, vpHeight);  

  glBindTexture(GL_TEXTURE_2D, this->TextureObjectRight->GetHandle());

  glBegin(GL_QUADS);
      glTexCoord2f(0.0f, 0.0f);
      glVertex3f(-1.0f, -1.0f, -1.0f);

      glTexCoord2f(1.0f, 0.0f);
      glVertex3f(1.0f, -1.0f, -1.0f);

      glTexCoord2f(1.0f, 1.0f);
      glVertex3f(1.0f, 1.0f, -1.0f);

      glTexCoord2f(0, 1.0f);
      glVertex3f(-1.0f, 1.0f, -1.0f);
  glEnd();                

  glBindTexture(GL_TEXTURE_2D, 0);
  glDisable(GL_TEXTURE_2D);
 
  glPopAttrib();
  glPopMatrix();

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();

  glMatrixMode(GL_MODELVIEW); */

  // Restore former camera
  s2.GetRenderer()->SetActiveCamera(cam);
  cam->UnRegister(this);

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
}

// ----------------------------------------------------------------------------
// Description:
// Render delegate with a image of different dimensions than the
// original one.
// \pre s_exists: s!=0
// \pre fbo_exists: fbo!=0
// \pre fbo_has_context: fbo->GetContext()!=0
// \pre target_exists: target!=0
// \pre target_has_context: target->GetContext()!=0
void vtkDualStereoPass::RenderMS(const vtkRenderState *s,
                                            int fboWidth,
                                            int fboHeight,
                                            int vpX,
                                            int vpY,
                                            int vpWidth,
                                            int vpHeight,
                                            vtkTextureObject *target)
{
  assert("pre: s_exists" && s!=0);
  //assert("pre: fbo_exists" && fbo!=0);
  //assert("pre: fbo_has_context" && fbo->GetContext()!=0);
  assert("pre: target_exists" && target!=0);
  assert("pre: target_has_context" && target->GetContext()!=0);

  vtkRenderer *r=s->GetRenderer();  
  vtkRenderState s2(r);
  s2.SetPropArrayAndCount(s->GetPropArray(),s->GetPropArrayCount());

  int fboSize[2] = {fboWidth, fboHeight};
  vtkMyFBO *fakeFBO = vtkMyFBO::New();

  fakeFBO->SetLastSize(fboSize);
  s2.SetFrameBuffer(fakeFBO);
  //s2.SetFrameBuffer(fbo);
  

  /*fbo->SetNumberOfRenderTargets(1);
  fbo->SetColorBuffer(0,target);

  // because the same FBO can be used in another pass but with several color
  // buffers, force this pass to use 1, to avoid side effects from the
  // render of the previous frame.
  fbo->SetActiveBuffer(0);

  fbo->SetDepthBufferNeeded(false);
  fbo->StartNonOrtho(-1,-1,false);*/
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  r->GetRenderWindow()->MakeCurrent();
  vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT, this->Internals->MultiSampleFBO);

  glViewport(0, 0, fboWidth, fboHeight);
  glScissor(0, 0, fboWidth, fboHeight);
  
  // 2. Delegate render in FBO
  //glEnable(GL_DEPTH_TEST);
  this->DelegatePass->Render(&s2);  
    
  vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT, 0);  
  vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT, this->Internals->StereoResolveFBO);      
                 
  glEnable(GL_TEXTURE_2D);
  target->Bind();
  vtkgl::FramebufferTexture2DEXT(vtkgl::FRAMEBUFFER_EXT, vtkgl::COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, target->GetHandle(), 0);
  target->UnBind();
  glDisable(GL_TEXTURE_2D);
  
  vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT, 0);
  vtkgl::BindFramebufferEXT(vtkgl::READ_FRAMEBUFFER_EXT, this->Internals->MultiSampleFBO); 
  vtkgl::BindFramebufferEXT(vtkgl::DRAW_FRAMEBUFFER_EXT, this->Internals->StereoResolveFBO);

  vtkgl::BlitFramebuffer(0, 0, 
                           fboWidth, fboHeight,
                           0, 0, 
                           fboWidth, fboHeight,
                           GL_COLOR_BUFFER_BIT, 
                           GL_NEAREST);

  //this->StereoFrameBuffer->UnBind();
  vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT, 0);

  fakeFBO->Delete();  
}

// ----------------------------------------------------------------------------
// Description:
// Release graphics resources and ask components to release their own
// resources.
// \pre w_exists: w!=0
void vtkDualStereoPass::ReleaseGraphicsResources(vtkWindow *w)
{
  assert("pre: w_exists" && w!=0);

  if(this->Internals->FBOColorRB)
    {
    vtkgl::DeleteRenderbuffersEXT(1, &this->Internals->FBOColorRB);
    this->Internals->FBOColorRB = 0;
    }
  if(this->Internals->FBODepthRB)
    {
    vtkgl::DeleteRenderbuffersEXT(1, &this->Internals->FBODepthRB);
    this->Internals->FBODepthRB = 0;
    }  
  if(this->Internals->StereoResolveFBO)
    {
    vtkgl::DeleteFramebuffersEXT(1, &this->Internals->StereoResolveFBO);
    this->Internals->StereoResolveFBO = 0;
    }
  if(this->Internals->MultiSampleFBO)
    {
    vtkgl::DeleteFramebuffersEXT(1, &this->Internals->MultiSampleFBO);
    this->Internals->MultiSampleFBO = 0;
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
void vtkDualStereoPass::CheckFramebufferStatus()
{
  switch (vtkgl::CheckFramebufferStatus(vtkgl::FRAMEBUFFER_EXT))
    {
    case vtkgl::FRAMEBUFFER_COMPLETE_EXT:
        break;

    case vtkgl::FRAMEBUFFER_UNSUPPORTED_EXT:
        vtkWarningMacro( << "Setup FBO failed. Unsupported framebuffer format.");

    case vtkgl::FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
        vtkWarningMacro( << "Setup FBO failed. Missing attachment.");

    case vtkgl::FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
        vtkWarningMacro( << "Setup FBO failed. Duplicate attachment.");

    case vtkgl::FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
        vtkWarningMacro( << "Setup FBO failed. Attached images must have the same dimensions.");

    case vtkgl::FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
        vtkWarningMacro( << "Setup FBO failed. Attached images must have the same format.");

    case vtkgl::FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
        vtkWarningMacro( << "Setup FBO failed. Missing draw buffer.");

    case vtkgl::FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
        vtkWarningMacro( << "Setup FBO failed. Missing read buffer.");

    case vtkgl::FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT:
        vtkWarningMacro( << "Setup FBO failed. Attached images must have the same number of samples.");

    default:
        vtkWarningMacro( << "Setup FBO failed. Fatal error.");
    }
}

//----------------------------------------------------------------------------
void vtkDualStereoPass::PrintSelf(ostream& os, vtkIndent indent)
{
  Superclass::PrintSelf(os, indent);
}
