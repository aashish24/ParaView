/*=========================================================================

   Program:   ParaView
   Module:    pqOculusRiftView.cxx

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
#include "pqOculusRiftView.h"

#include "pqApplicationCore.h"
#include "pqInterfaceTracker.h"
#include "pqStandardViewModules.h"
#include "pqServer.h"

#include "vtkSMSessionProxyManager.h"
#include "vtkPVOptions.h"
#include "vtkSMProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMEnumerationDomain.h"

#include <QStringList>

namespace
{
  /// interface class for plugins that create view modules
  class VTK_EXPORT pqOculusRiftViewInterface : public pqStandardViewModules
  {
public:
  pqOculusRiftViewInterface(QObject* o) : pqStandardViewModules(o) { }
  ~pqOculusRiftViewInterface() { }

  virtual QStringList viewTypes() const
    {
    QStringList list;
    list << "RenderViewOculusRift";
    return list;
    }

  QString viewTypeName(const QString& val) const
    {
    return "Ocuslus Rift Render View";
    }

  virtual vtkSMProxy* createViewProxy(
    const QString& viewtype, pqServer *server)
    {
    vtkSMSessionProxyManager* pxm = server->proxyManager();
    vtkPVOptions* pvoptions = server->getOptions();
    if (viewtype == "RenderViewOculusRift")
      {
      return pxm->NewProxy("views", "RenderViewOculusRift");
      }
    return NULL;
    }
  };
}

//-----------------------------------------------------------------------------
pqOculusRiftView::pqOculusRiftView(QObject* p)
  : Superclass(p)
{
}

//-----------------------------------------------------------------------------
pqOculusRiftView::~pqOculusRiftView()
{
}

//-----------------------------------------------------------------------------
void pqOculusRiftView::onStartup()
{
  // Register ParaView interfaces.
  pqInterfaceTracker* pgm = pqApplicationCore::instance()->interfaceTracker();

  pqOculusRiftViewInterface *viewInterface = 
    new pqOculusRiftViewInterface(pgm);
  // * adds support for standard paraview views.
  pgm->addInterface(viewInterface);  
}

//-----------------------------------------------------------------------------
void pqOculusRiftView::onShutdown()
{
}