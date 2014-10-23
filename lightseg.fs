uniform sampler2D tex;
uniform vec3 col;
uniform vec2 emitA, emitB;
uniform vec2 casterA, casterB;
uniform vec2 screen;
in vec2 uv;

vec2 project(vec2 p, vec2 caster)
{
    vec2 ap = p-emitA;
    vec2 ab = emitB-emitA;
    vec2 d = caster-p;
    vec2 c = casterB - casterA;
    float s = sign(dot(c,ab));
    vec2 n = vec2(-ab.y, ab.x);
    float l = -dot(d,n);
    float m = dot(ap, ab) + (dot(d,ab)/max(l,0))*dot(ap, n);
    m /= dot(ab,ab);
    if (isinf(m)) return vec2(1./0., 0.0);
    return vec2(m,dot(n,ap)/max(l,0));
}
const float pi = 3.141592653;
void main(void)
{
    vec2 uv2 = gl_FragCoord.xy/screen;
    vec2 p = 2*uv2 - 1;
    vec2 c = casterB - casterA;
    vec2 N = vec2(-c.y, c.x);
    vec2 ab = emitB-emitA;
    vec2 mid = 0.5*(emitB+emitA);
    vec2 n = vec2(-ab.y, ab.x);
float s = sign(dot(c,ab));
    vec2 lA = project(p, casterA);
    vec2 lB = project(p, casterB);

    float o;float dbg = 0;
    if (lA.y < 1) {dbg=1;lA.x = 0;}
    if (lB.y < 1) {dbg=1;lB.x = 1;}
    if (lA.y < 1 && lB.y < 1) { lA.x = 1; }
    o = clamp(clamp(lB.x,0,1) + clamp(-lA.x,-1,0), 0, 1);
    if (dot(N,p-casterA) < 0) o = 0;
    if (lA.x >= lB.x)  o = 0;
    if (dot(n,p-emitA) < 0) o = 1;
    vec3 oo = vec3(0,0,0);
    /*if (lA >= 0 && lA <= 1) oo.x = 1;
    if (lB >= 0 && lB <= 1) oo.y = 1;
    if (lA <= 0 && lB >= 1) oo = (1).xxx;*/
    /*if (lA <= 0) oo.x = 1;
    if (lB >= 1) oo.y = 1;*/
    //float nt = max(clamp(-dot(normalize(emitB-p),normalize(n)), 0, 1), clamp(-dot(normalize(emitA-p),normalize(n)), 0, 1));
    float a1 = atan(emitB.y-p.y,emitB.x-p.x),
          a2 = atan(emitA.y-p.y,emitA.x-p.x);
    float nt = clamp(mod((a1-a2)/pi, 2),0,1);
    gl_FragColor = vec4(o.xxx, 1);

    //gl_FragColor = vec4(lA.x,lB.x,0, 1);
}
