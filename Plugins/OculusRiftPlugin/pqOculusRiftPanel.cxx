/*=========================================================================

   Program:   ParaView
   Module:    pqOculusRiftPanel.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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
#include "pqOculusRiftPanel.h"
#include "ui_pqOculusRiftPanel.h"

#include "vtkSMSessionProxyManager.h"
#include "vtkSMProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkCamera.h"
#include "vtkMath.h"

#include "pqApplicationCore.h"
#include "pqActiveObjects.h"
#include "pqServerManagerModel.h"
#include "pqInterfaceTracker.h"
#include "pqStandardViewModules.h"
#include "pqServer.h"
#include "pqView.h"
#include "pqRenderView.h"

#include <QThread>
#include <QStringList>
#include <QDebug>

#include "pqOculusRiftDevice.h"

class pqOculusRiftPanel::pqInternals : public Ui::OculusRiftPanel
{
public:
  pqInternals()
  {
    this->HScreenSize = 0;
    this->VScreenSize = 0;
    this->XScreenRes = 0;
    this->YScreenRes = 0;
    this->LensSeparationDistance = 0;
    this->ProjOffset[0] = 0;
    this->ProjOffset[1] = 0;
  }

  QPointer<pqRenderView> RenderView;
    
  pqOculusRiftDevice *OrDevice;
  QThread *OrThread;

  double HScreenSize;
  double VScreenSize;
  int XScreenRes;
  int YScreenRes;
  double LensSeparationDistance;
  double ProjOffset[2];
};

//-----------------------------------------------------------------------------
void pqOculusRiftPanel::constructor()
{
  this->setWindowTitle("Oculus Rift Options");
  QWidget* container = new QWidget(this);
  this->Internal = new pqInternals();
  this->Internal->setupUi(container);
  this->setWidget(container);

  this->deviceIntialized = 0;
  this->viewPortWidth = 0;
  this->viewPortHeight = 0;
  this->Tracking = false;
  this->UserFOV = -1.0;

  this->LastSensorEye[0] = 0;
  this->LastSensorEye[1] = 0;
  this->LastSensorEye[2] = 0;

  this->LastSensorLookAt[0] = 0;
  this->LastSensorLookAt[1] = 0;
  this->LastSensorLookAt[2] = 0;

  this->LastSensorUp[0] = 0;
  this->LastSensorUp[1] = 0;
  this->LastSensorUp[2] = 0;

  // Other
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)),
          this, SLOT(setView(pqView*)));

  /*QObject::connect(this->Internal->UpdateButton, SIGNAL(released()),
          this, SLOT(updateParameters()));*/
  QObject::connect(this->Internal->trackingButton, SIGNAL(toggled(bool)),
    this, SLOT(setTrackingEnabled(bool)));

  QObject::connect(this->Internal->stereoPostButton, SIGNAL(toggled(bool)),
    this, SLOT(setStereoPostEnabled(bool)));

  QObject::connect(this->Internal->fovSlider, SIGNAL(valueChanged(int)),
    this, SLOT(onChangeFOV(int)));

  // Add the render view to the proxy combo
  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();

  QList<pqRenderView*> rviews = ::pqFindItems<pqRenderView*>(smmodel);
  if (rviews.size() != 0)
    this->setView(rviews.first());

  this->Internal->OrDevice = new pqOculusRiftDevice();
  this->Internal->OrThread = new QThread();

  this->Internal->OrDevice->moveToThread(this->Internal->OrThread);

  if(this->Internal->OrDevice->initialize())
    {
    this->Internal->OrThread->start();
    QObject::connect(this->Internal->OrDevice, SIGNAL(updatedData()),
      this, SLOT(updateView()));
    }

  this->setTrackingEnabled(this->Tracking);

  this->Internal->trackingButton->setChecked(this->Tracking);

  QObject::connect(this->Internal->resetSensorButton, SIGNAL(released()),
    this->Internal->OrDevice, SLOT(calibrateSensor()));

  QObject::connect(this->Internal->sensorPredictionButton, SIGNAL(released()),
    this->Internal->OrDevice, SLOT(togglePrediction()));
}

//-----------------------------------------------------------------------------
pqOculusRiftPanel::~pqOculusRiftPanel()
{
  delete this->Internal->OrDevice;
  delete this->Internal->OrThread;
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqOculusRiftPanel::setView(pqView* view)
{
  if(!view)
    {
    return;
    }

  vtkSMProxy* proxy = view->getProxy();
  if(!proxy)
    {
    return;
    }

  // We are looking for our Oculus Render View proxy
  if(!strcmp(proxy->GetXMLName(), "RenderViewOculusRift"))
    {
    this->Internal->RenderView = qobject_cast<pqRenderView*>(view);
    if(this->Internal->RenderView)
      {
      // connect widgets to current render view
      this->connectGUI();
      if(this->deviceIntialized)
        {
        this->sendParameters();
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqOculusRiftPanel::updateView()
{
  if(!this->Internal->RenderView)
    {
    return;
    }

  vtkSMProxy* proxy = this->Internal->RenderView->getProxy();
  if(!proxy)
    {
    return;
    }
      
  if(!strcmp(proxy->GetXMLName(), "RenderViewOculusRift"))
    {
    QSize size = this->Internal->RenderView->getSize();
    if(this->viewPortWidth != size.width() ||
        this->viewPortHeight  != size.height())
      {
      this->viewPortWidth = size.width();
      this->viewPortHeight = size.height();
      /*this->Internal->OrDevice->setViewport(this->viewPortWidth, 
        this->viewPortHeight);*/

      this->Internal->OrDevice->getScreenResolution(this->Internal->XScreenRes,
                                          this->Internal->YScreenRes);
      this->Internal->OrDevice->getScreenSize(this->Internal->HScreenSize,
                                          this->Internal->VScreenSize);
      
      this->Internal->OrDevice->setViewport(this->Internal->XScreenRes,
        this->Internal->YScreenRes);

      double vFOV = this->Internal->OrDevice->getFieldOfView();

      if(this->UserFOV < 0)
        {
        this->UserFOV = vFOV;
        this->Internal->FOV->setText(QString::number(this->UserFOV)); 
        this->Internal->fovSlider->setValue(10 * this->UserFOV);
        }
      else
        {
        vFOV = this->UserFOV;
        }
      this->deviceIntialized = 1;
      this->sendParameters();

      this->Internal->trackingButton->setEnabled(true);
      this->Internal->stereoPostButton->setEnabled(true);
      }
    if(this->Tracking)
      {
      double userEyePos[3] = { 0,0,0};
      double eye[3], lookAt[3], up[3];
      double camEye[3], camLookAt[3], camUp[3];
      vtkCamera *camera = this->Internal->RenderView->
                            getRenderViewProxy()->GetActiveCamera();
      camera->GetPosition(camEye);
      camera->GetFocalPoint(camLookAt);
      camera->GetViewUp(camUp);

      this->Internal->OrDevice->getCameraSetup(userEyePos, eye, lookAt, up);

      camEye[0] += eye[0] - this->LastSensorEye[0];
      camEye[1] += eye[1] - this->LastSensorEye[1];
      camEye[2] += eye[2] - this->LastSensorEye[2];

      camLookAt[0] += lookAt[0] - this->LastSensorLookAt[0];
      camLookAt[1] += lookAt[1] - this->LastSensorLookAt[1];
      camLookAt[2] += lookAt[2] - this->LastSensorLookAt[2];

      camUp[0] += up[0] - this->LastSensorUp[0];
      camUp[1] += up[1] - this->LastSensorUp[1];
      camUp[2] += up[2] - this->LastSensorUp[2];

      vtkSMPropertyHelper(proxy, "CameraPosition").Set(camEye, 3);
      vtkSMPropertyHelper(proxy, "CameraFocalPoint").Set(camLookAt, 3);
      vtkSMPropertyHelper(proxy, "CameraViewUp").Set(camUp, 3);

      this->LastSensorEye[0] = eye[0];
      this->LastSensorEye[1] = eye[1];
      this->LastSensorEye[2] = eye[2];

      this->LastSensorLookAt[0] = lookAt[0];
      this->LastSensorLookAt[1] = lookAt[1];
      this->LastSensorLookAt[2] = lookAt[2];

      this->LastSensorUp[0] = up[0];
      this->LastSensorUp[1] = up[1];
      this->LastSensorUp[2] = up[2];
      }
    proxy->UpdateVTKObjects();
    this->Internal->RenderView->getViewProxy()->StillRender();
    } 
}

//-----------------------------------------------------------------------------
void pqOculusRiftPanel::connectGUI()
{
  if(!this->Internal->RenderView)
    {
    return;
    }

  vtkSMProxy* proxy = this->Internal->RenderView->getProxy();
  if(!proxy)
    {
    return;
    }

  this->blockSignals(true);
    
  this->StereoPostClient =
    vtkSMPropertyHelper(proxy, "StereoAndPostOnClient").GetAsInt();
  
  this->setStereoPostEnabled(this->StereoPostClient);
  this->Internal->stereoPostButton->setChecked(this->StereoPostClient);

  this->blockSignals(false);
}

//-----------------------------------------------------------------------------
void pqOculusRiftPanel::setTrackingEnabled(bool value)
{
  if(value)
    {
    this->Internal->trackingButton->setText(
      "Oculus Rift Tracking (enabled)");
    }
  else
    {
    this->Internal->trackingButton->setText(
      "Oculus Rift Tracking (disabled)");

    this->LastSensorEye[0] = 0;
    this->LastSensorEye[1] = 0;
    this->LastSensorEye[2] = 0;

    this->LastSensorLookAt[0] = 0;
    this->LastSensorLookAt[1] = 0;
    this->LastSensorLookAt[2] = 0;

    this->LastSensorUp[0] = 0;
    this->LastSensorUp[1] = 0;
    this->LastSensorUp[2] = 0;
    }
  
  this->Internal->fovSlider->setEnabled(value);
  this->Internal->resetSensorButton->setEnabled(value);
  this->Internal->sensorPredictionButton->setEnabled(value);

  this->Tracking = value;
}

//-----------------------------------------------------------------------------
void pqOculusRiftPanel::setStereoPostEnabled(bool value)
{
  if(value)
    {
    this->Internal->stereoPostButton->setText(
      "Stereo and Post-Process on Client (enabled)");
    }
  else
    {
    this->Internal->stereoPostButton->setText(
      "Stereo and Post-Process on Client (disabled)");
    }
  this->StereoPostClient = value;

  if(!this->Internal->RenderView)
    {
    return;
    }

  vtkSMProxy* proxy = this->Internal->RenderView->getProxy();
  if(!proxy)
    {
    return;
    }

  vtkSMPropertyHelper(proxy, "StereoAndPostOnClient").Set(value);
  proxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqOculusRiftPanel::onChangeFOV(int value)
{
  this->UserFOV = value / 10.0f;
  this->Internal->FOV->setText(QString::number(this->UserFOV));  
   
  vtkSMProxy* proxy = this->Internal->RenderView->getProxy();
  if(!proxy)
    {
    return;
    }

  vtkSMPropertyHelper(proxy, "CameraViewAngle").Set(this->UserFOV);
  proxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqOculusRiftPanel::sendParameters()
{
  if(!this->Internal->RenderView)
    {
    return;
    }

  vtkSMProxy* proxy = this->Internal->RenderView->getProxy();
  if(!proxy)
    {
    return;
    }

  double dist_k[4], chromaAb[4];
  this->Internal->OrDevice->getDistortionK(dist_k);
  this->Internal->OrDevice->getChromaAberration(chromaAb);
  double ipd = this->Internal->OrDevice->getIPD();
  double distScale = this->Internal->OrDevice->getDistortionScale();
  this->Internal->ProjOffset[0] = this->Internal->OrDevice->getDistortionCenter();

  vtkSMPropertyHelper(proxy, "EyeSeparation").Set(ipd);
  vtkSMPropertyHelper(proxy, "ProjectionOffset").Set(this->Internal->ProjOffset, 2);
  vtkSMPropertyHelper(proxy, "CameraViewAngle").Set(this->UserFOV);
  vtkSMPropertyHelper(proxy, "DistortionK").Set(dist_k, 4);
  vtkSMPropertyHelper(proxy, "DistortionScale").Set(distScale);
  vtkSMPropertyHelper(proxy, "ChromaAberation").Set(chromaAb, 4);

  proxy->UpdateVTKObjects();
}