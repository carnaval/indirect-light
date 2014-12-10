layout(binding=0) uniform sampler2D tex;
uniform vec3 col;
uniform vec2 lightpos;
in vec2 worldUv;
in vec2 uv;
in vec4 attr1;
void main(void)
{
    vec4 s = texture(tex, worldUv);
    vec2 N = attr1.zw;
    float f = dot(-N,normalize(lightpos-worldUv));
    float i = s.x*f;//s.y*(1-s.x);
    vec2 res = vec2(0, 1);
    if (i > 0) res = uv.yy;
    gl_FragColor = vec4(res, i, 1);
}
