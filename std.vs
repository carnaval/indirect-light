

in vec3 pos;
out vec2 uv;

void main(void)
{
    gl_Position = vec4(pos.xy, 0, pos.z);
    uv = pos.xy*0.5 + 0.5;
    //uv.y = 1-uv.y;
    //uv = uv.yx;
}
