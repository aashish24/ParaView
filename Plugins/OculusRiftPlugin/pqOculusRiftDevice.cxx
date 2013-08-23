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

#include <QTimer>
#include <QThread>
#include <QDebug>

// Oculus Rift SDK
#include "OVR.h"

class pqOculusRiftDevice::pqInternals
{
public:
  OVR::Ptr<OVR::DeviceManager> pManager;
  OVR::Ptr<OVR::HMDDevice> pHMD;
  OVR::Ptr<OVR::SensorDevice> pSensor;
  OVR::SensorFusion SFusion;
  OVR::Util::MagCalibration MagCal;
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
  
      this->Internals->pSensor = *this->Internals->pHMD->GetSensor();
      if (this->Internals->pSensor)
        {
        this->Internals->SFusion.AttachToSensor(this->Internals->pSensor);
        //this->Internals->SFusion.SetYawCorrectionEnabled(true);
        } 

      this->Internals->Timer = new QTimer();
      this->Internals->Timer->setInterval(1);

      connect(this->Internals->Timer, SIGNAL(timeout()), this, SLOT(timeoutSlot()));

      this->Internals->Timer->start();

      return true;
      }
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
      OVR::Util::Render::Viewport(0,0, 1280, 800));
    this->Internals->Stereo.SetStereoMode(OVR::Util::Render::Stereo_LeftRight_Multipass);

    // Configure proper Distortion Fit.
    // For 7" screen, fit to touch left side of the view, leaving a bit of invisible
    // screen on the top (saves on rendering cost).
    // For smaller screens (5.5"), fit to the top.
    if (hmd.HScreenSize > 0.0f)
      {
      if (hmd.HScreenSize > 0.140f) // 7"
        this->Internals->Stereo.SetDistortionFitPointVP(-1.0f, 0.0f);
      else
        this->Internals->Stereo.SetDistortionFitPointVP(0.0f, 1.0f);
      }
    }
}

//-----------------------------------------------------------------------------
void pqOculusRiftDevice::getScreenResolution(int &xRes, int &yRes)
{
  OVR::HMDInfo hmd;
  if (this->Internals->pHMD->GetDeviceInfo(&hmd))
    {
    xRes = hmd.HResolution;
    yRes = hmd.VResolution;
    }
}


//-----------------------------------------------------------------------------
void pqOculusRiftDevice::getScreenSize(double &hSize, double &vSize)
{
  OVR::HMDInfo hmd;
  if (this->Internals->pHMD->GetDeviceInfo(&hmd))
    {
    hSize = hmd.HScreenSize;
    vSize = hmd.VScreenSize;
    }
}

//-----------------------------------------------------------------------------
double pqOculusRiftDevice::getLensSeparation()
{
  OVR::HMDInfo hmd;
  if (this->Internals->pHMD->GetDeviceInfo(&hmd))
    {
    return hmd.LensSeparationDistance;
    }
  return 0.0;
}

//-----------------------------------------------------------------------------
double pqOculusRiftDevice::getIPD()
{
  OVR::HMDInfo hmd;
  if (this->Internals->pHMD->GetDeviceInfo(&hmd))
    {
    return this->Internals->Stereo.GetIPD();
    }
  return 0.0;
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
void pqOculusRiftDevice::getChromaAberration(double chromaAb[4])
{
  OVR::HMDInfo hmd;
  if (this->Internals->pHMD->GetDeviceInfo(&hmd))
    {
    chromaAb[0] = hmd.ChromaAbCorrection[0];
    chromaAb[1] = hmd.ChromaAbCorrection[1];
    chromaAb[2] = hmd.ChromaAbCorrection[2];
    chromaAb[3] = hmd.ChromaAbCorrection[3];
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

//-----------------------------------------------------------------------------
void pqOculusRiftDevice::getEulerAngles(float *yaw, float *pitch, float *roll)
{
  // We extract Yaw, Pitch, Roll instead of directly using the orientation
  // to allow "additional" yaw manipulation with mouse/controller.
  OVR::Quatf hmdOrient = this->Internals->SFusion.GetOrientation();
  
  hmdOrient.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(
    yaw, pitch, roll);
}

//-----------------------------------------------------------------------------
void pqOculusRiftDevice::getCameraSetup(double camEye[3], double eye[3], double lookAt[3], double up[3])
{
  // We extract Yaw, Pitch, Roll instead of directly using the orientation
  // to allow "additional" yaw manipulation with mouse/controller.
  OVR::Quatf hmdOrient = this->Internals->SFusion.GetPredictedOrientation();

  const OVR::Vector3f UpVector(0.0f, 1.0f, 0.0f);
  const OVR::Vector3f ForwardVector(0.0f, 0.0f, -1.0f);

  OVR::Vector3f EyePos(camEye[0], camEye[1], camEye[2]);
  
  float yaw = 0.0;
  float pitch = 0.0;
  float roll = 0.0;
  
  hmdOrient.GetEulerAngles<OVR::Axis_Y, OVR::Axis_X, OVR::Axis_Z>(&yaw, &pitch, &roll);
  
  // Rotate and position View Camera, using YawPitchRoll in BodyFrame coordinates.
  // 
  OVR::Matrix4f rollPitchYaw = OVR::Matrix4f::RotationY(yaw) * OVR::Matrix4f::RotationX(pitch) *
                          OVR::Matrix4f::RotationZ(roll);
  OVR::Vector3f upVec      = rollPitchYaw.Transform(UpVector);
  OVR::Vector3f forwardVec = rollPitchYaw.Transform(ForwardVector);
      
  // Minimal head modelling.
  float headBaseToEyeHeight     = 0.15f;    // Vertical height of eye from base of head
  float headBaseToEyeProtrusion = 0.09f;    // Distance forward of eye from base of head

  OVR::Vector3f eyeCenterInHeadFrame(0.0f, headBaseToEyeHeight, -headBaseToEyeProtrusion);
  OVR::Vector3f shiftedEyePos   = EyePos + rollPitchYaw.Transform(eyeCenterInHeadFrame);
  shiftedEyePos.y -= eyeCenterInHeadFrame.y;  // Bring the head back down to original height
  
  eye[0] = shiftedEyePos.x;
  eye[1] = shiftedEyePos.y;
  eye[2] = shiftedEyePos.z;

  shiftedEyePos += forwardVec;

  lookAt[0] = shiftedEyePos.x;
  lookAt[1] = shiftedEyePos.y;
  lookAt[2] = shiftedEyePos.z;

  up[0] = upVec.x;
  up[1] = upVec.y;
  up[2] = upVec.z;
}

//-----------------------------------------------------------------------------
void pqOculusRiftDevice::timeoutSlot()
{   
  if (this->Internals->MagCal.IsAutoCalibrating() || 
    this->Internals->MagCal.IsManuallyCalibrating())
    this->Internals->MagCal.AbortCalibration();

  if (this->Internals->MagCal.IsAutoCalibrating()) 
    {
    this->Internals->MagCal.UpdateAutoCalibration(this->Internals->SFusion);
    int n = this->Internals->MagCal.NumberOfSamples();
    if (n == 1)
      qWarning() << "Magnetometer Calibration Has 1 Sample \n " << (4-n) 
                  << " Remaining - Please Keep Looking Around";
    else if (n < 4)
      qWarning() << "Magnetometer Calibration Has " << n 
                  <<" Samples   \n   " << (4-n) 
                  << " Remaining - Please Keep Looking Around";
    if (this->Internals->MagCal.IsCalibrated())
      {
      this->Internals->SFusion.SetYawCorrectionEnabled(true);
      }
    }

  emit updatedData();
}

//-----------------------------------------------------------------------------
void pqOculusRiftDevice::calibrateSensor()
{ 
  if(this->Internals->pSensor)
    {
    this->Internals->SFusion.Reset();
    this->Internals->MagCal.BeginAutoCalibration(this->Internals->SFusion);    
    }
}

//-----------------------------------------------------------------------------
void pqOculusRiftDevice::togglePrediction()
{ 
  if(this->Internals->pSensor)
    {
    if(this->Internals->SFusion.IsPredictionEnabled())
      {
          qWarning() << "Turn off prediction";
          this->Internals->SFusion.SetPredictionEnabled(false);
      }
    else
      {
        qWarning() << "Turn on prediction";
        this->Internals->SFusion.SetPredictionEnabled(true);
        this->Internals->SFusion.SetPrediction(0.03f);
      }
    }
}

//-----------------------------------------------------------------------------
void pqOculusRiftDevice::stop()
{
  this->Internals->Timer->stop();
  this->thread()->wait();  
}