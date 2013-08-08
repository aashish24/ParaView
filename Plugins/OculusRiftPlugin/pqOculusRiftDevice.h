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
#ifndef __pqOculusRiftDevice_h
#define __pqOculusRiftDevice_h

#include <QObject>

class pqOculusRiftDevice : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;
public:
  pqOculusRiftDevice(QObject* p=0);
  ~pqOculusRiftDevice();

  bool initialize();

  void getOrientation(double orientationMatrix[16]);
  void getEulerAngles(float &yaw, float &pitch, float &roll);

  void setViewport(int width, int height);

  double getFieldOfView();
  void getDistortionK(double K[4]);
  double getDistortionCenter();
  double getDistortionScale();
  double getEyeToScreenDistance();
  double getDeviceAspectRatio();

  void stop();

signals:
  void updatedData();

protected slots:
  void timeoutSlot();

private:
  pqOculusRiftDevice(const pqOculusRiftDevice&); // Not implemented.
  void operator=(const pqOculusRiftDevice&); // Not implemented.

  int WindowWidth;
  int WindowHeight;

  float ScaleIn; //Rescale input texture coordinates to [-1,1] unit range, and corrects aspectratio.
  float Scale; //Rescale output (sample) coordinates back to texture range and increase scale so as to support sampling outside of the screen.
  float HmdWarpParam; //The array of distortion coefficients (DistortionK[]).
  float ScreenCenter; //Texture coordinate for the center of the half-screen texture. This is used to clamp sampling, preventing pixel leakage from one eye view to the other.
  float LensCenter; // Shifts texture coordinates to center the distortion function around the center of the lens.

  class pqInternals;
  pqInternals* Internals;
};
#endif
