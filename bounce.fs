layout(binding=0) uniform sampler2D tex;
uniform vec3 col;
in vec2 worldUv;
in vec2 uv;
void main(void)
{
    float i = texture(tex, worldUv).y;
    vec2 res = vec2(0, 1);
    if (i >= 0.5) res = uv.yy;
    gl_FragColor = vec4(res, 0, 1);
}
