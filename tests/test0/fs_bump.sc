$input v_wpos, v_view, v_texcoord0 // in...

/*
 * Copyright 2011-2015 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include "../common/common.sh"

SAMPLER2D(u_texColor, 0);

void main()
{
    vec3 view = -normalize(v_view);
    vec4 color = toLinear(texture2D(u_texColor, v_texcoord0) );

    gl_FragColor.xyz = color.xyz;
    gl_FragColor.w = 1.0;
    gl_FragColor = toGamma(gl_FragColor);
}
