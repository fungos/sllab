$input a_position, a_texcoord0
$output v_wpos, v_view, v_texcoord0

/*
 * Copyright 2011-2015 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "../common/common.sh"

void main()
{
    vec3 wpos = mul(u_model[0], vec4(a_position, 1.0) ).xyz;
    gl_Position = mul(u_viewProj, vec4(wpos, 1.0) );

    v_wpos = wpos;

    vec3 view = mul(u_view, vec4(wpos, 0.0) ).xyz;
    v_view = view;

    v_texcoord0 = a_texcoord0;
}
