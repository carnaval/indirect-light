layout(binding=0) uniform sampler2D tex;
uniform vec3 col;
in vec2 uv;

void main(void)
{
    vec4 tx = texture(tex, uv);
    gl_FragColor = vec4(tx.xyz, 1);
}
