layout(binding=0) uniform sampler2D tex;
uniform vec3 col;
in vec2 uv;

void main(void)
{
    gl_FragColor = vec4(texture(tex, uv).xyz, 1);
}
