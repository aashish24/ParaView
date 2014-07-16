// ============================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkOVRPostShaderSimple_fs.glsl
//
//  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
//  All rights reserved.
//  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
//
//     This software is distributed WITHOUT ANY WARRANTY; without even
//     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//     PURPOSE.  See the above copyright notice for more information.
//
// ============================================================================
/*----------------------------------------------------------------------
Acknowledgement:
This contribution has been developed at the "Brandenburg University of
Technology Cottbus - Senftenberg" at the chair of "Media Technology."
Implemented by Stephan ROGGE
------------------------------------------------------------------------*/

#version 110

uniform sampler2D source;

uniform vec2 LensCenter;
uniform vec2 ScreenCenter;
uniform vec2 Scale;
uniform vec2 ScaleIn;
uniform vec4 HmdWarpParam;

vec2 HmdWarp(vec2 texIn)
{
  vec2 theta = (texIn - LensCenter) * ScaleIn;
  float  rSq= theta.x * theta.x + theta.y * theta.y;
  vec2 theta1 = theta * (HmdWarpParam.x + HmdWarpParam.y * rSq +
       HmdWarpParam.z * rSq * rSq + HmdWarpParam.w * rSq * rSq * rSq);
  return LensCenter + Scale * theta1; //theta1;
}

void main()
{
  vec2 tc = HmdWarp(gl_TexCoord[0].st);
  if (any(notEqual(clamp(tc, ScreenCenter-vec2(0.5,0.5), ScreenCenter+vec2(0.5, 0.5)) - tc, vec2(0.0, 0.0))))
    gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
  else
    gl_FragColor = texture2D(source, tc);
};
