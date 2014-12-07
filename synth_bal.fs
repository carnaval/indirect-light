layout(binding=0) uniform sampler2D tex;
uniform vec3 col;
in vec2 uv;
uniform vec2 a;
const float pi = 3.141592653;
uniform float r,E;
void main(void)
{
    vec4 s = texture(tex, uv);
    float o = 1-clamp(s.x,0,1);
    vec2 p = uv.xy*2 - 1;
    float dist = length(p-a);
    float I = clamp(1/r,0,1)/(dist*(1+dist));
    float l = /* I* */o*E;
    gl_FragColor = vec4(l,0,0, 1);
}
