// ============================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkOVRPostShaderFull_fs.glsl
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
uniform vec4 ChromAbParam;

// Scales input texture coordinates for distortion.
// ScaleIn maps texture coordinates to Scales to ([-1, 1]), although top/bottom will be
// larger due to aspect ratio.
void main()
{
	vec2  theta = (gl_TexCoord[0].st - LensCenter) * ScaleIn; // Scales to [-1, 1]
	float rSq= theta.x * theta.x + theta.y * theta.y;
	vec2  theta1 = theta * (HmdWarpParam.x + HmdWarpParam.y * rSq +
		      HmdWarpParam.z * rSq * rSq + HmdWarpParam.w * rSq * rSq * rSq);

	// Detect whether blue texture coordinates are out of range since these will scaled out the furthest.
	vec2 thetaBlue = theta1 * (ChromAbParam.z + ChromAbParam.w * rSq);
	vec2 tcBlue = LensCenter + Scale * thetaBlue;
	if (!all(equal(clamp(tcBlue, ScreenCenter-vec2(0.5,0.5),  ScreenCenter+vec2(0.5, 0.5)) - tcBlue, vec2(0.0, 0.0))))
	//if (!all(equal(clamp(tcBlue, ScreenCenter-vec2(0.25,0.5), ScreenCenter+vec2(0.25,0.5)), tcBlue)))
	{
	   gl_FragColor = vec4(0);
	   return;
	}

	// Now do blue texture lookup.
	float blue = texture2D(source, tcBlue).b;

	// Do green lookup (no scaling).
	vec2  tcGreen = LensCenter + Scale * theta1;
	vec4  center = texture2D(source, tcGreen);

	// Do red scale and lookup.
	vec2  thetaRed = theta1 * (ChromAbParam.x + ChromAbParam.y * rSq);
	vec2  tcRed = LensCenter + Scale * thetaRed;
	float red = texture2D(source, tcRed).r;

	gl_FragColor = vec4(red, center.g, blue, center.a);
};
