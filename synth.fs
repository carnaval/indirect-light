layout(binding=0) uniform sampler2D tex;
uniform vec3 col;
in vec2 uv;

void main(void)
{
    vec4 s = texture(tex, uv);
    float o = s.x;
    float I = clamp(s.y, 0, 1);
    float l = 1-clamp(o,0,1);
    gl_FragColor = vec4(l,0,0, 1);
}
