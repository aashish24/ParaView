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
#include "vtkMatrix4x4.h"
#include "vtkTransform.h"
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
  QPointer<pqRenderView> RenderView;
    
  pqOculusRiftDevice *OrDevice;
  QThread *OrThread;
};

//-----------------------------------------------------------------------------
void pqOculusRiftPanel::constructor()
{
  this->setWindowTitle("Oculus Rift Options");
  QWidget* container = new QWidget(this);
  this->Internal = new pqInternals();
  this->Internal->setupUi(container);
  this->setWidget(container);

  this->viewPortWidth = 0;
  this->viewPortHeight = 0;
  this->Tracking = false;

  // Other
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)),
          this, SLOT(setView(pqView*)));

  /*QObject::connect(this->Internal->UpdateButton, SIGNAL(released()), 
          this, SLOT(updateParameters()));*/
  QObject::connect(this->Internal->trackingButton, SIGNAL(toggled(bool)),
    this, SLOT(setTrackingEnabled(bool)));

  QObject::connect(this->Internal->trackingClientButton, SIGNAL(toggled(bool)),
    this, SLOT(setTrackingClientEnabled(bool)));

  QObject::connect(this->Internal->stereoPostButton, SIGNAL(toggled(bool)),
    this, SLOT(setStereoPostEnabled(bool)));

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
  this->Internal->trackingClientButton->setEnabled(this->Tracking);
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
        this->viewPortHeight  != size.width())
      {
      this->viewPortWidth = size.width();
      this->viewPortHeight = size.height();
      this->Internal->OrDevice->setViewport(this->viewPortWidth, 
        this->viewPortHeight);

      int dim[2] = {size.width()/2, size.height()};
      double dist_k[4];
      double distScale, distXOffset;
      this->Internal->OrDevice->getDistortionK(dist_k);
      distScale = this->Internal->OrDevice->getDistortionScale();
      distXOffset = this->Internal->OrDevice->getDistortionCenter();
      double vFOV = this->Internal->OrDevice->getFieldOfView();

      double d = this->Internal->OrDevice->getEyeToScreenDistance();
      double as = this->Internal->OrDevice->getDeviceAspectRatio();
      double displaySettings[3];
      displaySettings[2] = d;
      displaySettings[1] = tan( vtkMath::RadiansFromDegrees( vFOV ) / 2.0) * d;
      displaySettings[0] = displaySettings[1] * as;
        
      vtkSMPropertyHelper(proxy, "DisplaySettings").Set(displaySettings, 3);
      vtkSMPropertyHelper(proxy, "DistortionK").Set(dist_k, 4);
      vtkSMPropertyHelper(proxy, "DistortionScale").Set(distScale);
      vtkSMPropertyHelper(proxy, "DistortionXCenterOffset").Set(distXOffset);
      vtkSMPropertyHelper(proxy, "OffscreenTextureDim").Set(dim, 2);
      }
    if(this->Tracking)
      {
      vtkTransform *orientation = vtkTransform::New();
      double matrix[16];
        
      this->Internal->OrDevice->getOrientation(matrix);

      orientation->SetMatrix(matrix); 
      orientation->Inverse();   

      vtkSMPropertyHelper(proxy, "HeadOrientation").Set(
        &orientation->GetMatrix()->Element[0][0], 16);
      }
    proxy->UpdateVTKObjects();      
    this->Internal->RenderView->forceRender();
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
    
  this->TrackingClient = 
    vtkSMPropertyHelper(proxy, "TrackingOnClient").GetAsInt();
  this->StereoPostClient = 
    vtkSMPropertyHelper(proxy, "StereoAndPostOnClient").GetAsInt();

  this->setTrackingClientEnabled(this->TrackingClient);
  this->Internal->trackingClientButton->setChecked(this->TrackingClient);

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
    this->Internal->trackingClientButton->setEnabled(true);
    }
  else
    {
    this->Internal->trackingButton->setText(
      "Oculus Rift Tracking (disabled)");
    this->Internal->trackingClientButton->setEnabled(false);
    }

  this->Tracking = value;
}

//-----------------------------------------------------------------------------
void pqOculusRiftPanel::setTrackingClientEnabled(bool value)
{
  if(value)
    {
    this->Internal->trackingClientButton->setText(
      "Oculus Tracker on Client (enabled)");    
    }
  else
    {
    this->Internal->trackingClientButton->setText(
      "Oculus Tracker on Client (disabled)");
    }

  this->TrackingClient = value;

  if(!this->Internal->RenderView)
    {
    return;
    }   

  vtkSMProxy* proxy = this->Internal->RenderView->getProxy();
  if(!proxy)
    {
    return;
    }

  vtkSMPropertyHelper(proxy, "TrackingOnClient").Set(value);
  proxy->UpdateVTKObjects();
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