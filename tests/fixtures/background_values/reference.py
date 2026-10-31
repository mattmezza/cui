#!/usr/bin/env python3
"""Offline independent double-precision goldens; never invokes CUI.
CSS Color 4 premultiplied interpolation; CSS Images gradient-line projection.
SOURCE_038 projection/straight alpha: gpui-pre-wgpu0.3.8 shaders.wgsl,
https://docs.rs/crate/gpui-pre-wgpu/0.3.8/source/src/shaders.wgsl
extracted SHA2561d91f2c8a1958a241f1b659a4137c160592c8076b9be09049db4c9fb9a344529.
Run explicitly to review fixture changes; tests consume committed numbers only.
"""
import math

def mul(matrix, vector):
    return [sum(a*b for a,b in zip(row, vector)) for row in matrix]

LMS = [[.4122214708,.5363325363,.0514459929], [.2119034982,.6806995451,.1073969566], [.0883024619,.2817188376,.6299787005]]
LAB = [[.2104542553,.793617785,-.0040720468], [1.9779984951,-2.428592205,.4505937099], [.0259040371,.7827717662,-.808675766]]
LMS_INV = [[1,.3963377774,.2158037573], [1,-.1055613458,-.0638541728], [1,-.0894841775,-1.291485548]]
RGB_INV = [[4.0767416621,-3.3077115913,.2309699292], [-1.2684380046,2.6097574011,-.3413193965], [-.0041960863,-.7034186147,1.707614701]]
def encode(x):
    return x*12.92 if x <= .0031308 else 1.055*x**(1/2.4)-.055

def decode(x):
    return x/12.92 if x <= .04045 else ((x+.055)/1.055)**2.4

def sample(source, lab, angle, width, height, x, y, a, b):
    direction = [math.sin(math.radians(angle)), -math.cos(math.radians(angle))]
    if source:
        scale = min(width,height)
        direction = [direction[0]*scale/height, direction[1]*scale/width]
        extent = math.hypot(*direction) * (width if abs(direction[0]) > abs(direction[1]) else height)
    else:
        extent = sum(abs(d*s) for d,s in zip(direction,[width,height]))
    position = .5+sum(d*p for d,p in zip(direction,[x-width/2,y-height/2]))/extent
    t = min(1,max(0,(position-.25)/.5))
    alpha = a[3]*(1-t)+b[3]*t
    def forward(color):
        return mul(LAB,[v**(1/3) for v in mul(LMS,color[:3])]) if lab else list(map(encode,color[:3]))
    ca, cb = forward(a), forward(b)
    mixed = [(u*(1-t)+v*t) if source else ((u*a[3]*(1-t)+v*b[3]*t)/alpha if alpha else 0) for u,v in zip(ca,cb)]
    rgb = mul(RGB_INV,[v**3 for v in mul(LMS_INV,mixed)]) if lab else list(map(decode,mixed))
    return [v*alpha for v in rgb]+[alpha]

print('/* Frozen independent reference.py output; see script provenance. */')
for source in range(2):
    for lab in range(2):
        for angle,w,h,x,y in [(90,120,40,60,20),(45,120,40,30,30),(135,40,120,30,30),(225,120,40,90,10),(315,40,120,10,90)]:
            a,b = [1,0,0,.25],[0,0,1,.75]
            expected = sample(source,lab,angle,w,h,x,y,a,b)
            print('{%d,%d,%d,%d,%d,%d,%d,{%s}},' % (source,lab,angle,w,h,x,y,','.join('%.10ff'%v for v in expected)))
