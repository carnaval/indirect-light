layout(binding=0) uniform sampler2D tex;
uniform vec3 col;
in vec2 uv;
uniform vec2 a,b;
const float pi = 3.141592653;
uniform float E;
void main(void)
{
    vec4 s = texture(tex, uv);
    float o = 1-clamp(s.x,0,1);
    vec2 p = uv.xy*2 - 1;
    vec2 d = b-a;
    vec2 n = vec2(-d.y,d.x);
    float a1 = atan(b.y-p.y,b.x-p.x),
          a2 = atan(a.y-p.y,a.x-p.x);
    vec2 mid = 0.5*a+0.5*b;
    float sA = clamp(mod((a1-a2)/(pi), 1),0,1);
    //float I = clamp(mod((a1-a2)/(pi), 1),0,1)*sign(max(dot(n,p-a),0))/dot(mid-p,mid-p);
    //float I = clamp(-dot(mid-p,n)/(length(d))/(1+sqrt(dot(a-p,a-p)*dot(b-p,b-p))),0,1)*(1+sA);
    float I = clamp(-dot(mid-p,n)/(length(d))/(1+dot(a-p,a-p)*dot(b-p,b-p)),0,1)*(1+sA)*(1+sA)*(1+sA);
    float l = I*o*E;
    gl_FragColor = vec4(l,0,0, 1);
}
