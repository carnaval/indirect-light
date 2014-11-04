layout(binding=0) uniform sampler2D tex;
uniform vec3 col;
in vec2 uv;

void main(void)
{
    vec4 s = texture(tex, uv);
    /*float o = s.x;
    float I = s.y;*/
    gl_FragColor = vec4(s.xyz, 1);
}
