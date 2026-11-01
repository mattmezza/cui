// SPDX-License-Identifier: MIT
'use strict';
// One owned JS schema over the exact optional C bridge group; no native views.
module.exports = function sliderModelModule(native, Node, NODE, keyFor) {
  const receipts = new WeakMap();
  const appNamespaces = new WeakMap();
  const freeze = Object.freeze;
  const uint = (x, max = 0xffffffff) => {
    if (!Number.isInteger(x) || x < 0 || x > max) throw new TypeError('expected Slider unsigned integer');
    return x;
  };
  const big = x => {
    if (typeof x !== 'bigint' || x < 0n || x > 0xffffffffffffffffn) throw new TypeError('expected uint64 Slider BigInt');
    return x;
  };
  const number = x => { if (typeof x !== 'number' || !Number.isFinite(x)) throw new TypeError('expected finite Slider double'); return x; };
  const bool = x => { if (typeof x !== 'boolean') throw new TypeError('expected Slider boolean'); return x; };
  const enumValue = (x, values) => { if (!values.includes(x)) throw new TypeError('unknown Slider vocabulary'); return x; };
  function slider_single(end) { return freeze({kind: 'single', end: number(end)}); }
  function slider_range(start, end) {
    number(start); number(end);
    if (start > end) throw new RangeError('Slider range must be ordered');
    return freeze({kind: 'range', start, end});
  }
  function value(v) {
    if (v?.kind === 'single') return slider_single(v.end);
    if (v?.kind === 'range') return slider_range(v.start, v.end);
    throw new TypeError('expected tagged Slider Single/Range');
  }
  function domain(v = {}) {
    const d = {minimum: number((v.minimum === undefined ? 0 : v.minimum)), maximum: number((v.maximum === undefined ? 100 : v.maximum)), step: number((v.step === undefined ? 1 : v.step)), scale: enumValue((v.scale === undefined ? 'linear' : v.scale), ['linear','logarithmic'])};
    if (d.minimum > d.maximum || !Number.isFinite(d.maximum-d.minimum) || d.step < 0 || (d.scale === 'logarithmic' && (d.minimum <= 0 || d.maximum <= d.minimum))) throw new RangeError('invalid Slider domain');
    return freeze(d);
  }
  function slider_config(v = {}) {
    const layout = enumValue((v.layout === undefined ? 'fixed' : v.layout), ['fixed','parent']);
    const rem = Math.fround(number((v.rem === undefined ? 16 : v.rem)));
    const length = layout === 'parent' ? 0 : Math.fround(number((v.length === undefined ? 200 : v.length)));
    if (!Number.isFinite(rem) || rem <= 0 || !Number.isFinite(length) || (layout === 'fixed' && length <= 0)) throw new RangeError('invalid Slider float32 geometry');
    return freeze({domain: domain(v.domain), orientation: enumValue((v.orientation === undefined ? 'horizontal' : v.orientation),['horizontal','vertical']), layout, length, rem,
      sourceGeneration: big((v.sourceGeneration === undefined ? 0n : v.sourceGeneration)), requests: bool((v.requests === undefined ? true : v.requests)), readonly: bool((v.readonly === undefined ? false : v.readonly)), reverseFill: bool((v.reverseFill === undefined ? false : v.reverseFill)), reducedMotion: bool((v.reducedMotion === undefined ? false : v.reducedMotion)), hideFocus: bool((v.hideFocus === undefined ? false : v.hideFocus))});
  }
  function pair(v, d) {
    v = value(v);
    if (v.end < d.minimum || v.end > d.maximum || (v.kind === 'range' && (v.start < d.minimum || v.start > d.maximum))) throw new RangeError('Slider value outside domain');
    return v;
  }
  const wireValue = v => [v.kind === 'single' ? 0 : 1, v.kind === 'single' ? 0 : v.start, v.end];
  const wireConfig = c => [(c.requests?1:0)|(c.readonly?2:0)|(c.reverseFill?4:0)|(c.reducedMotion?8:0)|(c.hideFocus?16:0),c.orientation==='vertical'?1:0,c.sourceGeneration,c.domain.minimum,c.domain.maximum,c.domain.step,c.domain.scale==='logarithmic'?1:0,c.rem,c.length,c.layout==='parent'?1:0];
  function fromValue(a) {
    if (!Array.isArray(a) || a.length !== 3) throw new TypeError('invalid Slider value copy');
    return enumValue(a[0],[0,1]) === 0 ? (a[1] === 0 ? slider_single(a[2]) : (()=>{throw new TypeError('nonzero inactive Slider start');})()) : slider_range(a[1],a[2]);
  }
  function fromDomain(a) {
    if (!Array.isArray(a) || a.length !== 4) throw new TypeError('invalid Slider domain copy');
    return domain({minimum:a[0],maximum:a[1],step:a[2],scale:enumValue(a[3],[0,1])===0?'linear':'logarithmic'});
  }
  function fromConfig(a) {
    if (!Array.isArray(a) || a.length !== 10 || uint(a[0],31) !== a[0] || (a[9] === 1 && a[8] !== 0)) throw new TypeError('invalid Slider config copy');
    return slider_config({domain:{minimum:a[3],maximum:a[4],step:a[5],scale:enumValue(a[6],[0,1])?'logarithmic':'linear'},orientation:enumValue(a[1],[0,1])?'vertical':'horizontal',sourceGeneration:a[2],rem:a[7],length:a[8],layout:enumValue(a[9],[0,1])?'parent':'fixed',requests:!!(a[0]&1),readonly:!!(a[0]&2),reverseFill:!!(a[0]&4),reducedMotion:!!(a[0]&8),hideFocus:!!(a[0]&16)});
  }
  function call(app, op, node = null, a = null, b = null) {
    app._alive();
    if (typeof native.sliderModel !== 'function') throw new Error('controlled Slider addon unavailable');
    return native.sliderModel(app.handle,op,node,a,b);
  }
  function supports(app) { return typeof native.sliderModel === 'function' && call(app,0); }
  function enable(app, capacity = 4096) {
    uint(capacity,4096); call(app,1,null,capacity);
    if (!appNamespaces.has(app)) appNamespaces.set(app,freeze({}));
    app.sliderEnabled = true; return app;
  }
  function copyRequest(app, a) {
    if (!appNamespaces.has(app)) throw new TypeError('Slider requests require successful queue enable');
    if (!Array.isArray(a) || a.length !== 15) throw new TypeError('invalid Slider request copy');
    const phase = enumValue(a[0],[0,1,2,3]), reason = uint(a[1],9), thumb = enumValue(a[2],[0,1]);
    const eventKind = enumValue(a[3],[1,3,5]), eventSource = uint(a[4],4), eventFlags = uint(a[5],7);
    const r = a[6], i = a[14];
    if (!Array.isArray(r) || r.length !== 7 || !Array.isArray(i) || i.length !== 14) throw new TypeError('invalid Slider receipt/input copy');
    const receipt = freeze({appEpoch:big(r[0]),node:freeze({slot:uint(r[1]),generation:uint(r[2])}),sourceGeneration:big(r[3]),configurationRevision:big(r[4]),submissionFrame:big(r[5]),requestSequence:big(r[6])});
    const eventTarget=big(a[7]),eventFrame=big(a[8]),eventIndex=big(a[9]);
    const current=fromValue(a[10]),requested=fromValue(a[11]),d=fromDomain(a[12]),gestureDomain=fromDomain(a[13]);
    pair(current,d); // CANCEL may intentionally retain an older gesture-domain requested pair.
    const input = freeze({type:enumValue(i[0],[0,1,2,3,7,13]),flags:uint(i[1],3),key:enumValue(i[2],[0,4,5,6,7,8,9,10,19,20]),modifiers:uint(i[3],15),x:number(i[4]),y:number(i[5]),repeat:uint(i[6],1),pointerType:uint(i[7],2),pointerButton:uint(i[8],2),accessibilityAction:enumValue(i[9],[0,2,3,4]),pointerId:big(i[10]),pointerDevice:big(i[11]),accessibilityTarget:big(i[12]),accessibilityValue:number(i[13])});
    if ((!(eventFlags&1) && eventFrame!==0n) || (!(eventFlags&2) && eventIndex!==0xffffffffffffffffn) || (!(eventFlags&4) && eventSource!==0) || ((input.flags&2) && (!(input.flags&1) || input.type!==13 || input.accessibilityAction!==4))) throw new TypeError('incoherent Slider evidence');
    if (!(input.flags&1) && i.some((v,index)=>index>=10&&index<=12?v!==0n:v!==0)) throw new TypeError('nonzero absent Slider input');
    if (!(input.flags&2) && input.accessibilityValue!==0) throw new TypeError('nonzero inactive Slider numeric evidence');
    receipts.set(receipt,{namespace:appNamespaces.get(app),wire:freeze([...r])});
    return freeze({phase,reason,thumb,eventKind,eventSource,eventFlags,receipt,eventTarget,eventFrame,eventIndex,current,requested,domain:d,gestureDomain,input});
  }
  function poll(app) { const a=call(app,7);return a===undefined?undefined:copyRequest(app,a); }
  function accept(app, receipt, v) {
    const token=receipts.get(receipt);
    if (!appNamespaces.has(app) || !token || token.namespace!==appNamespaces.get(app)) throw new TypeError('foreign or reconstructed Slider receipt');
    call(app,8,null,token.wire,wireValue(value(v)));
  }
  function dispatch(app) {
    if (app.sliderDispatching) throw new Error('nested controlled Slider dispatch unavailable');
    app.sliderDispatching=true; let delivered=0;
    try {
      while (!app.closed && !app.disposed) {
        const request=poll(app); if (request===undefined) break;
        const node=app.nodes.get(keyFor(request.receipt.node)); const callback=node?.requestValue;
        if (callback) callback(request);
        delivered++;
      }
    } finally {app.sliderDispatching=false;}
    return delivered;
  }
  class SliderModel extends Node {
    constructor(key,config,confirmed,text='Selection') {
      super(81,text); this.key(key); this.configValue=slider_config(config);this.confirmedValue=pair(confirmed,this.configValue.domain);this.requestValue=null;
    }
    on_request(callback) {if(callback!==null&&typeof callback!=='function')throw new TypeError('expected Slider callback or null');this.requestValue=callback;return this;}
    configure(config,confirmed) {
      const c=slider_config(config),v=pair(confirmed,c.domain);
      if(this.owner)call(this.owner,3,this[NODE],wireConfig(c),wireValue(v));
      this.configValue=c;this.confirmedValue=v;return this;
    }
    set_value(confirmed) {
      const v=pair(confirmed,this.configValue.domain);
      if(v.kind!==this.confirmedValue.kind)throw new TypeError('set_value cannot change Slider kind');
      if(this.owner)call(this.owner,5,this[NODE],wireValue(v));
      this.confirmedValue=v;return this;
    }
    get_value() {return this.owner?fromValue(call(this.owner,6,this[NODE])):this.confirmedValue;}
    get_config() {return this.owner?fromConfig(call(this.owner,4,this[NODE])):this.configValue;}
    part_bounds(part) {uint(part,4);if(!this.owner)throw new Error('Slider is not mounted');const a=call(this.owner,10,this[NODE],part);if(a===undefined)return undefined;const b={x:number(a[0]),y:number(a[1]),width:number(a[2]),height:number(a[3])};if(b.width<0||b.height<0)throw new TypeError('invalid Slider bounds');return freeze(b);}
    presentation() {if(!this.owner)throw new Error('Slider is not mounted');const a=call(this.owner,9,this[NODE]);if(a===undefined)return undefined;const p={renderStatus:uint(a[0],5),coreTime:number(a[1]),nextWakeSeconds:number(a[2])};if(p.coreTime<0||p.nextWakeSeconds<0)throw new TypeError('invalid Slider presentation clock');return freeze(p);}
    disabled(v=true) {bool(v);if(this.owner)native.setDisabled(this.owner.handle,this[NODE],v?1:0);this.disabledValue=v;return this;}
    on_change() {throw new TypeError('controlled Slider has no legacy CHANGE callback');}
  }
  function slider_model(key,config,confirmed,text='Selection') {return new SliderModel(key,config,confirmed,text);}
  function preflight(app,root) {
    let typed=false;
    const walk=(node,ancestors)=>{
      if(node.kind===81){if(!(node instanceof SliderModel)||ancestors.some(k=>k!==1&&k!==2))throw new TypeError('controlled Slider requires Row/Column ancestry');typed=true;}
      for(const child of node.childrenValue)walk(child,[...ancestors,node.kind]);
    };
    walk(root,[]);
    if(typed){if(!supports(app))throw new Error('controlled Slider V1 unavailable');if(!app.sliderEnabled)enable(app);}
  }
  function mount(app,node,parent) {return call(app,2,parent,node.keyValue,[wireConfig(node.configValue),wireValue(node.confirmedValue)]);}
  return {exports:{SliderModel,slider_model,slider_single,slider_range,slider_config},preflight,mount,supports,enable,poll,accept,dispatch};
};
