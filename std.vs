layout(location=0) in vec3 pos;
layout(location=1) in vec4 worldPos;
out vec2 uv;
out vec2 worldUv;
out vec4 attr1;

void main(void)
{
    gl_Position = vec4(pos.xy, 0, pos.z);
    uv = pos.xy*0.5 + 0.5;
    //if (abs(pos.z) < 0.0001) uv /= (abs(uv.x) + abs(uv.y));
    worldUv = worldPos.xy*0.5 + 0.5;
    attr1 = worldPos;
    //uv.y = 1-uv.y;
    //uv = uv.yx;
}
