"""Owned typed root shadow values. Preset/CSS factories call the canonical C bridge."""
from __future__ import annotations

import ctypes as C
from dataclasses import dataclass
from enum import IntEnum
import math

class ShadowKind(IntEnum):
    DROP = 0
    INSET = 1

class ShadowPreset(IntEnum):
    NONE = 0
    XXS = 1
    XS = 2
    SM = 3
    MD = 4
    LG = 5
    XL = 6
    XXL = 7

def _number(value, maximum, minimum=0):
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise TypeError('shadow values must be numbers')
    try:
        converted = C.c_float(value).value
    except (OverflowError, TypeError) as error:
        raise ValueError('shadow float32 overflow') from error
    if not math.isfinite(value) or not math.isfinite(converted) or not minimum <= value <= maximum:
        raise ValueError('shadow value outside supported domain')
    return converted

@dataclass(frozen=True)
class CornerRadii:
    top_left: float = 0
    top_right: float = 0
    bottom_left: float = 0
    bottom_right: float = 0

    def __post_init__(self):
        for key in self.__dataclass_fields__:
            object.__setattr__(self, key, _number(getattr(self, key), 16384))

class _Layer(C.Structure):
    _fields_ = [(n, C.c_float) for n in ('offset_x', 'offset_y', 'sigma', 'spread')] + [('kind', C.c_uint32), ('rgba', C.c_uint32)]

class _Style(C.Structure):
    _fields_ = [(n, C.c_uint32) for n in ('struct_size', 'schema_version', 'count', 'reserved')] + [(n, C.c_float) for n in ('top_left', 'top_right', 'bottom_left', 'bottom_right')] + [('layers', _Layer * 8)]

@dataclass(frozen=True)
class BoxShadow:
    offset_x: float = 0
    offset_y: float = 0
    sigma: float = 0
    spread: float = 0
    kind: ShadowKind = ShadowKind.DROP
    rgba: int = 0x00000040

    def __post_init__(self):
        for key, maximum, minimum in [('offset_x',4096,-4096),('offset_y',4096,-4096),('sigma',256,0),('spread',4096,-4096)]:
            object.__setattr__(self,key,_number(getattr(self,key),maximum,minimum))
        if not isinstance(self.kind, ShadowKind) or type(self.rgba) is not int or not 0 <= self.rgba <= 0xffffffff:
            raise ValueError('typed shadow kind and uint32 RGBA required')

    @classmethod
    def css(cls, offset_x=0, offset_y=0, blur_radius=0, spread=0, *, rgba=0x00000040, kind=ShadowKind.DROP):
        return CSSBoxShadow(offset_x,offset_y,blur_radius,spread,kind,rgba)

@dataclass(frozen=True)
class CSSBoxShadow:
    offset_x: float = 0
    offset_y: float = 0
    blur_radius: float = 0
    spread: float = 0
    kind: ShadowKind = ShadowKind.DROP
    rgba: int = 0x00000040

    def __post_init__(self):
        checked = BoxShadow(self.offset_x,self.offset_y,0,self.spread,self.kind,self.rgba)
        object.__setattr__(self,'offset_x',checked.offset_x)
        object.__setattr__(self,'offset_y',checked.offset_y)
        object.__setattr__(self,'spread',checked.spread)
        object.__setattr__(self,'blur_radius',_number(self.blur_radius,512))

    def _wire(self,api):
        out = _Layer()
        api.check(api.shadow_css(self.offset_x,self.offset_y,self.blur_radius,self.spread,self.rgba,int(self.kind),24,C.byref(out)))
        return out

@dataclass(frozen=True)
class ShadowStyle:
    layers: tuple[BoxShadow | CSSBoxShadow, ...] = ()
    radii: CornerRadii = CornerRadii()

    def __post_init__(self):
        copied = tuple(self.layers)
        if len(copied)>8 or not all(isinstance(v,(BoxShadow,CSSBoxShadow)) for v in copied) or not isinstance(self.radii,CornerRadii):
            raise ValueError('at most eight typed shadows and CornerRadii required')
        object.__setattr__(self,'layers',copied)

    def _wire(self,api):
        out = _Style(struct_size=224,schema_version=1,count=len(self.layers))
        for name in self.radii.__dataclass_fields__:
            setattr(out,name,getattr(self.radii,name))
        for i,v in enumerate(self.layers):
            out.layers[i] = v._wire(api) if isinstance(v,CSSBoxShadow) else _Layer(v.offset_x,v.offset_y,v.sigma,v.spread,int(v.kind),v.rgba)
        return out

    @classmethod
    def preset(cls,preset, radii=CornerRadii()):
        return PresetShadowStyle(preset,radii)

@dataclass(frozen=True)
class PresetShadowStyle:
    preset: ShadowPreset
    radii: CornerRadii = CornerRadii()

    def __post_init__(self):
        if not isinstance(self.preset,ShadowPreset) or not isinstance(self.radii,CornerRadii):
            raise ValueError('ShadowPreset and CornerRadii required')

    def _wire(self,api):
        out = _Style()
        r = self.radii
        api.check(api.shadow_preset(int(self.preset),r.top_left,r.top_right,r.bottom_left,r.bottom_right,224,C.byref(out)))
        return out

def bind_shadow_api(api, bind, handle_type):
    def available(name,restype,*args):
        try:
            return bind(name,restype,*args)
        except AttributeError:
            return None
    size = available('box_shadows_v1_size',C.c_uint32)
    def optional(name, restype, *args):
        fn = available(name,restype,*args)
        def call(*values):
            if fn is None or size is None:
                raise RuntimeError('bridge shadow V1 requires a matching newer CUI library')
            if size()!=224 or C.sizeof(_Style)!=224 or C.sizeof(_Layer)!=24 or _Style.layers.offset!=32 or _Layer.rgba.offset!=20:
                raise RuntimeError('bridge shadow V1 layout mismatch')
            return fn(*values)
        return call
    api.set_box_shadows_v1 = optional('set_box_shadows_v1',C.c_int,C.c_void_p,handle_type,C.c_uint32,C.POINTER(_Style))
    api.reset_box_shadows = optional('reset_box_shadows',C.c_int,C.c_void_p,handle_type)
    api.shadow_css = optional('box_shadow_from_css_v1',C.c_int,C.c_float,C.c_float,C.c_float,C.c_float,C.c_uint32,C.c_uint32,C.c_uint32,C.POINTER(_Layer))
    api.shadow_preset = optional('shadow_preset_v1',C.c_int,C.c_uint32,C.c_float,C.c_float,C.c_float,C.c_float,C.c_uint32,C.POINTER(_Style))
