uniform sampler2D tex;
uniform vec3 col;
in vec2 uv;

void main(void)
{
    gl_FragColor = vec4(col, 1);
}
