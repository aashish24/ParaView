# This plugin demonstrates how to add new render views to ParaView that use
# different render passes.
# It adds two views
# 1> Render View with Shadow Maps and
# 2> Render View with Edge detection

# Find and Include ParaView
if(NOT ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
else()
  include_directories(${VTK_INCLUDE_DIRS})
endif()

SET (QT_USE_QTNETWORK TRUE)
INCLUDE(${QT_USE_FILE})

SET (OR_SDK_INCLUDE_DIR "" CACHE PATH "Path to Oculus Rift SDK include files.")
SET (OR_SDK_LIBRARY "" CACHE FILEPATH "Path to Oculus Rift SDK library file.")

INCLUDE_DIRECTORIES(
  ${OR_SDK_INCLUDE_DIR}	
  )

QT4_WRAP_CPP(MOC_SRCS
  pqOculusRiftDevice.h
  pqOculusRiftView.h
  pqOculusRiftPanel.h
  )

QT4_WRAP_UI(UI_SRCS
  pqOculusRiftPanel.ui
  )

SET( OR_SOURCE_FILES
  pqOculusRiftDevice.h
  pqOculusRiftDevice.cxx
  pqOculusRiftView.h
  pqOculusRiftView.cxx
  pqOculusRiftPanel.h
  pqOculusRiftPanel.cxx
  vtkDualStereoPass.h
  vtkDualStereoPass.cxx
  vtkPVRenderViewOculusRift.h 
  vtkPVRenderViewOculusRift.cxx  
  ${MOC_SRCS})

encode_files_as_strings(ENCODED_STRING_FILES
   vtkLensCorrectionPassShader_fs.glsl
   )

ADD_PARAVIEW_AUTO_START(
  IFACES
  IFACE_SRCS
  CLASS_NAME pqOculusRiftView
  STARTUP onStartup
  SHUTDOWN onShutdown
  )

ADD_PARAVIEW_DOCK_WINDOW(
  IFACES_WINDOW IFACES_SRCS_WINDOW
  CLASS_NAME pqOculusRiftPanel
  DOCK_AREA Left
  )

add_paraview_plugin(
  OculusRiftPlugin "1.0"
  REQUIRED_ON_SERVER
  REQUIRED_ON_CLIENT
  SERVER_MANAGER_SOURCES
    vtkPVRenderViewOculusRift.cxx
  SERVER_MANAGER_XML
    OculusRiftView.xml
  GUI_INTERFACES
    ${IFACES}
    ${IFACES_WINDOW}
  GUI_SOURCES
    ${MOC_SRCS}
    ${UI_SRCS}
    ${RCS_SRCS} 
    ${IFACE_SRCS} 
    ${IFACES_SRCS_WINDOW}
    ${OR_SOURCE_FILES}
  SOURCES
    vtkLensCorrectionPass.cxx
    ${ENCODED_STRING_FILES}
  )

TARGET_LINK_LIBRARIES (OculusRiftPlugin pqApplicationComponents ${OR_SDK_LIBRARY} winmm.lib)