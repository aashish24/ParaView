/*=========================================================================

   Program:   ParaView
   Module:    pqOculusRiftDevice.h

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
#include "pqOculusRiftDevice.h"

#include <QTimer.h>
#include <QThread.h>
#include <QDebug.h>

// Oculus Rift SDK
#include "OVR.h"

class pqOculusRiftDevice::pqInternals
{
public:
  OVR::Ptr<OVR::DeviceManager> pManager;
  OVR::Ptr<OVR::HMDDevice> pHMD;
  OVR::Ptr<OVR::SensorDevice> pSensor;
  OVR::SensorFusion SFusion;
  OVR::Util::Render::StereoConfig Stereo;

  QTimer *Timer;

  float LastSensorYaw;
  float LastSensorPitch;
  float LastSensorRoll;
};


//-----------------------------------------------------------------------------
pqOculusRiftDevice::pqOculusRiftDevice(QObject* p)
  : Superclass(p)
{
  this->Internals = new pqInternals();

  this->Internals->pManager = NULL;
  this->Internals->pHMD = NULL;

  this->Internals->LastSensorYaw = 0.0f;
  this->Internals->LastSensorPitch = 0.0f;
  this->Internals->LastSensorRoll = 0.0f;

}

//-----------------------------------------------------------------------------
pqOculusRiftDevice::~pqOculusRiftDevice()
{
  delete this->Internals;

  // Destroy OVR
  OVR::System::Destroy();
}

//-----------------------------------------------------------------------------
bool pqOculusRiftDevice::initialize()
{
  // Initialize OVR
  OVR::System::Init(OVR::Log::ConfigureDefaultLog(OVR::LogMask_All));
    
  this->Internals->pManager = *OVR::DeviceManager::Create();
  this->Internals->pHMD = 
    *this->Internals->pManager->EnumerateDevices<OVR::HMDDevice>().
      CreateDevice();

  if(this->Internals->pHMD)
    {  
    OVR::HMDInfo hmd;
    if (this->Internals->pHMD->GetDeviceInfo(&hmd))
      {
    /*  this->InterpupillaryDistance = hmd.InterpupillaryDistance;
      this->DistortionK[0] = hmd.DistortionK[0];
      this->DistortionK[1] = hmd.DistortionK[1];
      this->DistortionK[2] = hmd.DistortionK[2];
      this->DistortionK[3] = hmd.DistortionK[3];
      this->EyeToScreenDistance = hmd.EyeToScreenDistance;
      this->LensSeparationDistance = hmd.*/
      //Rescale input texture coordinates to [-1,1] unit range, and corrects aspectratio.
      this->ScaleIn = 1.0 - 2.0 * hmd.LensSeparationDistance / hmd.HScreenSize;
      this->Scale = 1.0; //Rescale output (sample) coordinates back to texture range and increase scale so as to support sampling outside of the screen.
      this->HmdWarpParam; //The array of distortion coefficients (DistortionK[]).
      this->ScreenCenter; //Texture coordinate for the center of the half-screen texture. This is used to clamp sampling, preventing pixel leakage from one eye view to the other.
      this->LensCenter;  
     // qWarning() << hmd.HScreenSize << ", "  << hmd.VScreenSize;
      }
  
    this->Internals->pSensor = *this->Internals->pHMD->GetSensor();
  
    if (this->Internals->pSensor)
      {
      this->Internals->SFusion.AttachToSensor(this->Internals->pSensor);
      this->Internals->SFusion.SetYawCorrectionEnabled(true);
      } 

    this->Internals->Timer = new QTimer();
    this->Internals->Timer->setInterval(1);

    connect(this->Internals->Timer, SIGNAL(timeout()), this, SLOT(timeoutSlot()));

    this->Internals->Timer->start();

    return true;
    }
  else
    {
    qWarning() << "No Oculus Rift Device was found.";    
    }

  return false;
}


//-----------------------------------------------------------------------------
void pqOculusRiftDevice::setViewport(int width, int height)
{
  OVR::HMDInfo hmd;
  if (this->Internals->pHMD->GetDeviceInfo(&hmd))
    {
    this->Internals->Stereo.SetFullViewport(
      OVR::Util::Render::Viewport(0,0, width, height));
    /*this->Internals->Stereo.SetStereoMode(
      OVR::Util::Render::Stereo_LeftRight_Multipass);
    this->Internals->Stereo.SetHMDInfo(hmd);
    this->Internals->Stereo.SetDistortionFitPointVP(-1.0f, 0.0f);*/
    }
}

//-----------------------------------------------------------------------------
double pqOculusRiftDevice::getFieldOfView()
{
  OVR::HMDInfo hmd;
  if (this->Internals->pHMD->GetDeviceInfo(&hmd))
    {
    return this->Internals->Stereo.GetYFOVDegrees();
    }
  return 0.0;
}

//-----------------------------------------------------------------------------
void pqOculusRiftDevice::getDistortionK(double K[4])
{
  OVR::HMDInfo hmd;
  if (this->Internals->pHMD->GetDeviceInfo(&hmd))
    {
    K[0] = this->Internals->Stereo.GetDistortionK(0);
    K[1] = this->Internals->Stereo.GetDistortionK(1);
    K[2] = this->Internals->Stereo.GetDistortionK(2);
    K[3] = this->Internals->Stereo.GetDistortionK(3);
    }
}

//-----------------------------------------------------------------------------
double pqOculusRiftDevice::getDistortionCenter()
{
  OVR::HMDInfo hmd;
  if (this->Internals->pHMD->GetDeviceInfo(&hmd))
    {
    return this->Internals->Stereo.GetDistortionConfig().XCenterOffset;
    }
  return 0.0;
}

//-----------------------------------------------------------------------------
double pqOculusRiftDevice::getDistortionScale()
{
  OVR::HMDInfo hmd;
  if (this->Internals->pHMD->GetDeviceInfo(&hmd))
    {
    return this->Internals->Stereo.GetDistortionScale();
    }
  return 1.0;
}

//-----------------------------------------------------------------------------
double pqOculusRiftDevice::getEyeToScreenDistance()
{
  OVR::HMDInfo hmd;
  if (this->Internals->pHMD->GetDeviceInfo(&hmd))
    {
    return hmd.EyeToScreenDistance;
    }
  return 0.0;
}

//-----------------------------------------------------------------------------
double pqOculusRiftDevice::getDeviceAspectRatio()
{
  OVR::HMDInfo hmd;
  if (this->Internals->pHMD->GetDeviceInfo(&hmd))
    {
    return double(hmd.HResolution) / hmd.VResolution;
    }
  return 1.0;
}

//-----------------------------------------------------------------------------
void pqOculusRiftDevice::getOrientation(double orientationMatrix[16])
{
  float EyeYaw = 0.0f;
  float EyePitch = 0.0f;
  float EyeRoll = 0.0f;

  // We extract Yaw, Pitch, Roll instead of directly using the orientation
  // to allow "additional" yaw manipulation with mouse/controller.
  OVR::Quatf hmdOrient = this->Internals->SFusion.GetOrientation();
  
/*  hmdOrient.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(
    &EyeYaw, &EyePitch, &EyeRoll);

  EyeYaw += (EyeYaw - this->Internals->LastSensorYaw);
  this->Internals->LastSensorYaw = EyeYaw;
  */
  // NOTE: We can get a matrix from orientation as follows:
  OVR::Matrix4f hmdMat(hmdOrient);

  int c = 0;
  for(int i = 0; i < 4; i++)
  {
    for(int j = 0; j < 4; j++)
    {
      orientationMatrix[c++] = hmdMat.M[i][j];
    }
  }  
}

void pqOculusRiftDevice::getEulerAngles(float &yaw, float &pitch, float &roll)
{
  // We extract Yaw, Pitch, Roll instead of directly using the orientation
  // to allow "additional" yaw manipulation with mouse/controller.
  OVR::Quatf hmdOrient = this->Internals->SFusion.GetOrientation();
  
  hmdOrient.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(
    &yaw, &pitch, &roll);
}

//-----------------------------------------------------------------------------
void pqOculusRiftDevice::timeoutSlot()
{
  emit updatedData();
}

//-----------------------------------------------------------------------------
void pqOculusRiftDevice::stop()
{
  this->Internals->Timer->stop();
  this->thread()->wait();  
}