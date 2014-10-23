uniform sampler2D tex;
uniform vec3 col;
uniform vec2 origin;
in vec2 uv;

void main(void)
{
    vec2 e = origin - (2*uv - 1);
    float d = 1./(1 + length(e));
    gl_FragColor = vec4(col, 1);
}
