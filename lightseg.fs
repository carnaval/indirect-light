uniform sampler2D tex;
uniform vec2 emitA, emitB;
//uniform vec2 casterA, casterB;
in vec4 attr1;
uniform vec2 screen;
uniform float il;

const float pi = 3.141592653;
void main(void)
{
    vec2 casterA = attr1.xy, casterB = attr1.zw;
    vec2 uv2 = gl_FragCoord.xy/screen;
    vec2 p = 2*uv2 - 1;

    vec2 ab = emitB-emitA;
    vec2 ap = p-emitA;
    vec2 bp = p-emitB;
    vec2 sp = p-casterA;
    vec2 ep = p-casterB;
    vec2 se = casterB-casterA;

    float s = sign(ap.x*bp.y-ap.y*bp.x);
    float s2 = sign(sp.x*ep.y-sp.y*ep.x);
    float ino = 1./dot(ab,ab);
    vec2 n_ab = vec2(-ab.y, ab.x);
    vec2 n_se = vec2(-se.y, se.x);
    float ts = dot(ap,n_ab)/dot(sp,n_ab);
    float te = dot(ap,n_ab)/dot(ep,n_ab);
    float prs = dot(ap-ts*sp, ab)*ino;
    float pre = dot(ap-te*ep, ab)*ino;
    prs = clamp(prs, 0, 1); pre = clamp(pre, 0, 1);

    if (ts < 1) prs = 0;
    if (te < 1) pre = 1;
    if (dot(casterA-emitA,n_ab) < 0 && dot(casterB-emitA,n_ab) < 0) { prs = 1; pre = 0;}
    if (dot(emitA-casterA,n_se) > 0 && dot(emitB-casterA,n_se) > 0) { prs = 1; pre = 0;}
    if (s2 < 0) { prs = 1; pre = 0; }
    if (ts < 0 && te < 0) { prs = 1; pre = 0; }
    if (s < 0) { prs = 0; pre = 1; }

    float a1 = atan(emitB.y-p.y,emitB.x-p.x),
          a2 = atan(emitA.y-p.y,emitA.x-p.x);
    float nt = clamp(mod((a1-a2)/pi, 2),0,1);
    gl_FragColor = vec4((clamp(pre-prs,0,1)),il*nt/length(ab),0, 1);
    //gl_FragColor = vec4((s),(s2),0, 1);

//    gl_FragColor = vec4(1,attr1.x,attr1.y, 1);
}
