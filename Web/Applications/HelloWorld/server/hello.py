"""
    This module is a ParaViewWeb server application.
    The following command line illustrate how to use it::

        $ pvpython hello.py --file <filename>
                            --ds-host <hostname>
                            --ds-port <port>
                            --rs-host <hostname>
                            --rs-port <port>

        --ds-host None
             Host name where pvserver has been started

        --ds-port 11111
              Port number to use to connect to pvserver

        --rs-host None
              Host name where renderserver has been started

        --rs-port 22222
              Port number to use to connect to the renderserver

    Any ParaViewWeb executable script come with a set of standard arguments that
    can be overriden if need be::

        --port 8080
             Port number on which the HTTP server will listen to.

        --content /path-to-web-content/
             Directory that you want to server as static web content.
             By default, this variable is empty which mean that we rely on another server
             to deliver the static content and the current process only focus on the
             WebSocket connectivity of clients.

        --authKey vtkweb-secret
             Secret key that should be provided by the client to allow it to make any
             WebSocket communication. The client will assume if none is given that the
             server expect "vtkweb-secret" as secret key.

"""

# import to process args
import os
import sys
import base64

# import paraview modules.
from paraview.web import wamp      as pv_wamp
from paraview.web import protocols as pv_protocols

from vtk.web import server

try:
    import argparse
except ImportError:
    # since  Python 2.6 and earlier don't have argparse, we simply provide
    # the source for the same as _argparse and we use it instead.
    import _argparse as argparse


from autobahn.wamp import exportRpc

class CustomImageDelievery(pv_protocols.ParaViewWebProtocol):
  def __init__(self):
    self._netcdfFile = None
    self._initRender = False

  def setFileName(self, filename):
    self._netcdfFile = filename

  @exportRpc("initRender")
  def initRender(self):
    print 'init render called'
    reply = {"message": "success"}
    self._initRender = True
    return reply

  @exportRpc("stillRender")
  def stillRender(self, options):
    with open(self._netcdfFile, "rb") as image_file:
        imageString = base64.b64encode(image_file.read())

    reply = {}
    if not self._initRender:
        return reply

    reply['image'] = imageString
    reply['state'] = True
    reply['mtime'] = ""
    reply['size'] = [200, 200]
    reply['format'] = "jpeg;base64"
    reply['global_id'] = ""
    reply['localTime'] = ""
    reply['workTime'] = ""
    return reply

class _PipelineManager(pv_wamp.PVServerProtocol):

    dataDir = None
    authKey = "vtkweb-secret"
    dsHost = None
    dsPort = 11111
    rsHost = None
    rsPort = 11111
    fileToLoad = None

    def setFileName(self, filename):
        self.fileToLoad = filename

    def initialize(self):
        # Bring used components
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebStartupRemoteConnection(_PipelineManager.dsHost, _PipelineManager.dsPort, _PipelineManager.rsHost, _PipelineManager.rsPort))
        # self.registerVtkWebProtocol(pv_protocols.ParaViewWebStateLoader(_PipelineManager.fileToLoad))
        # self.registerVtkWebProtocol(pv_protocols.ParaViewWebPipelineManager(_PipelineManager.fileToLoad))
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebMouseHandler())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPort())
        # self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPortImageDelivery())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPortGeometryDelivery())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebTimeHandler())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebRemoteConnection())

        self._imageDelivery = CustomImageDelievery()
        self._imageDelivery.setFileName(_PipelineManager.fileToLoad)
        self.registerVtkWebProtocol(self._imageDelivery)

        # Update authentication key to use
        self.updateSecret(_PipelineManager.authKey)

# =============================================================================
# Main: Parse args and start server
# =============================================================================

if __name__ == "__main__":
    # Create argument parser
    parser = argparse.ArgumentParser(description="ParaView/Web Pipeline Manager web-application")

    # Add default arguments
    server.add_arguments(parser)

    # Add local arguments
    parser.add_argument("--file", default=None, help="File to load", dest="file")
    parser.add_argument("--ds-host", default=None, help="Hostname to connect to for DataServer", dest="dsHost")
    parser.add_argument("--ds-port", default=11111, type=int, help="Port number to connect to for DataServer", dest="dsPort")
    parser.add_argument("--rs-host", default=None, help="Hostname to connect to for RenderServer", dest="rsHost")
    parser.add_argument("--rs-port", default=11111, type=int, help="Port number to connect to for RenderServer", dest="rsPort")

    # Exctract arguments
    args = parser.parse_args()

    # Configure our current application
    _PipelineManager.authKey    = args.authKey
    _PipelineManager.dsHost     = args.dsHost
    _PipelineManager.dsPort     = args.dsPort
    _PipelineManager.rsHost     = args.rsHost
    _PipelineManager.rsPort     = args.rsPort
    if args.file is None:
        print 'At least one valid file is required'
    else:
        _PipelineManager.fileToLoad = args.file
    # Start server
    server.start_webserver(options=args, protocol=_PipelineManager)
