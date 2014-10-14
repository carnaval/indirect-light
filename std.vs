layout(location=0) in vec3 pos;
layout(location=1) in vec2 worldPos;
out vec2 uv;
out vec2 worldUv;

void main(void)
{
    gl_Position = vec4(pos.xy, 0, pos.z);
    uv = pos.xy*0.5 + 0.5;
    worldUv = worldPos*0.5 + 0.5;
    //uv.y = 1-uv.y;
    //uv = uv.yx;
}
