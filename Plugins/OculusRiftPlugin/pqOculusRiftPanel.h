/*=========================================================================

   Program:   ParaView
   Module:    pqOculusRiftPanel.h

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
#ifndef __pqOculusRiftPanel_h_
#define __pqOculusRiftPanel_h_

#include <QtGui/QDockWidget>

class pqView;
class vtkPerspectiveTransform;

class pqOculusRiftPanel : public QDockWidget
{
  Q_OBJECT
  typedef QDockWidget Superclass;
public:
  pqOculusRiftPanel(const QString &t, QWidget* p = 0, Qt::WindowFlags f=0):
    Superclass(t, p, f) { this->constructor(); }
  pqOculusRiftPanel(QWidget *p=0, Qt::WindowFlags f=0):
    Superclass(p, f) { this->constructor(); }
  virtual ~pqOculusRiftPanel();

protected slots:

  // set the view to show options for
  void setView(pqView* view);
  void updateView();
  void setTrackingEnabled(bool value);
  void setStereoPostEnabled(bool value);
  void onChangeFOV(int value);

protected:

  void connectGUI();
  void sendParameters();

private:
  pqOculusRiftPanel(const pqOculusRiftPanel&); // Not implemented.
  void operator=(const pqOculusRiftPanel&); // Not implemented.

  void constructor();

  int deviceIntialized;
  int viewPortWidth;
  int viewPortHeight;
  bool Tracking;
  bool StereoPostClient;
  float UserFOV;

  double LastSensorEye[3];
  double LastSensorLookAt[3];
  double LastSensorUp[3];

  class pqInternals;
  pqInternals* Internal;
};

#endif // __pqOculusRiftPanel_h_
