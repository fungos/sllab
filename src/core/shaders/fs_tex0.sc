$input v_texcoord0

#include "common.sh"

SAMPLER2D(u_texDiffuse, 0);

void main()
{
    gl_FragColor = toLinear(texture2D(u_texDiffuse, v_texcoord0)); // vec4(v_texcoord0.x, v_texcoord0.y, 1.0, 1.0);
}
