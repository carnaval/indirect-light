layout(binding=0) uniform sampler2D tex;
uniform vec3 col;
in vec2 worldUv;
in vec2 uv;
void main(void)
{
    vec4 s = texture(tex, worldUv);
    float i = s.y*(1-s.x);
    vec2 res = vec2(0, 1);
    if (i > 0) res = uv.yy;
    gl_FragColor = vec4(res, i, 1);
}
