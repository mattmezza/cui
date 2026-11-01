// SPDX-License-Identifier: MIT
'use strict';
// Explicit application-owned values over the optional complete C group.
module.exports = function localizedTimeModule(native, Node, NODE, keyFor) {
  const freeze=Object.freeze;
  const uint=(v,max=0xffffffff)=>{if(!Number.isInteger(v)||v<0||v>max)throw new TypeError('expected TimeField unsigned integer');return v;};
  const big=v=>{if(typeof v!=='bigint'||v<1n||v>0xffffffffffffffffn)throw new TypeError('expected nonzero uint64 TimeField BigInt');return v;};
  const bool=v=>{if(typeof v!=='boolean')throw new TypeError('expected TimeField boolean');return v;};
  const choice=(v,a)=>{if(!a.includes(v))throw new TypeError('unknown TimeField vocabulary');return v;};
  function utf8(s,max,empty=false){
    if(typeof s!=='string'||s.includes('\0')||(!empty&&!s.length))throw new TypeError('invalid TimeField text');
    for(const c of s){const n=c.codePointAt(0);if(n>=0xd800&&n<=0xdfff)throw new TypeError('unpaired UTF-16 surrogate');}
    const bytes=Buffer.from(s,'utf8');if(bytes.length>max)throw new RangeError('TimeField UTF-8 text too long');return bytes;
  }
  function time(v){if(!Array.isArray(v)||v.length!==3)throw new TypeError('expected [hour,minute,second]');return freeze([uint(v[0],23),uint(v[1],59),uint(v[2],59)]);}
  const seconds=v=>v[0]*3600+v[1]*60+v[2];
  function locale(v){
    if(!v||!Array.isArray(v.digits)||v.digits.length!==10)throw new TypeError('ten explicit TimeField digits required');
    const tokens=[...v.digits,v.separator,v.periodSeparator,v.am,v.pm];tokens.forEach(x=>utf8(x,16));
    for(let i=0;i<tokens.length;i++)for(let j=0;j<tokens.length;j++)if(i!==j&&tokens[j].startsWith(tokens[i]))throw new TypeError('TimeField tokens must be prefix-free');
    return freeze({digits:freeze(tokens.slice(0,10)),separator:tokens[10],periodSeparator:tokens[11],am:tokens[12],pm:tokens[13]});
  }
  function time_locale_english(){return locale({digits:[...'0123456789'],separator:':',periodSeparator:' ',am:'AM',pm:'PM'});}
  function time_locale_arabic(){return locale({digits:[...'٠١٢٣٤٥٦٧٨٩'],separator:':',periodSeparator:'\u202f',am:'ص',pm:'م'});}
  const fallback=(value,defaultValue)=>value===undefined?defaultValue:value;
  function localized_time_config(v={}){
    if(!v||typeof v!=='object'||Array.isArray(v))throw new TypeError('expected TimeField configuration');
    if(v.names!==undefined&&!Array.isArray(v.names))throw new TypeError('four TimeField segment names required');
    const c={locale:locale(fallback(v.locale,time_locale_english())),cycle:choice(fallback(v.cycle,'h23'),['h23','h12']),precision:choice(fallback(v.precision,'minute'),['minute','second']),
      minimum:v.minimum==null?null:time(v.minimum),maximum:v.maximum==null?null:time(v.maximum),editGeneration:big(fallback(v.editGeneration,1n)),
      disabled:bool(fallback(v.disabled,false)),readonly:bool(fallback(v.readonly,false)),ownerInvalid:bool(fallback(v.ownerInvalid,false)),names:freeze([...fallback(v.names,['','','',''])])};
    if(c.names.length!==4)throw new TypeError('four TimeField segment names required');c.names.forEach(s=>utf8(s,64,true));
    if(c.minimum&&c.maximum&&seconds(c.minimum)>seconds(c.maximum))throw new RangeError('TimeField bounds must be ordered');
    if(c.precision==='minute'&&[c.minimum,c.maximum].some(t=>t&&t[2]!==0))throw new RangeError('minute bounds require zero seconds');
    return freeze(c);
  }
  function encode(c){
    const b=Buffer.alloc(616);b.writeUInt32LE(616,0);b.writeUInt32LE(1,4);
    b.writeUInt32LE((c.disabled?1:0)|(c.readonly?2:0)|(c.ownerInvalid?4:0),8);
    b.writeUInt32LE(c.cycle==='h12'?1:0,12);b.writeUInt32LE(c.precision==='second'?1:0,16);
    b.writeUInt32LE((c.minimum?1:0)|(c.maximum?2:0),20);
    for(const [at,t] of [[32,c.minimum],[44,c.maximum]])if(t)t.forEach((n,i)=>b.writeUInt32LE(n,at+4*i));
    b.writeBigUInt64LE(c.editGeneration,56);
    [...c.locale.digits,c.locale.separator,c.locale.periodSeparator,c.locale.am,c.locale.pm].forEach((s,i)=>{const x=utf8(s,16);b.writeUInt32LE(x.length,64+20*i);x.copy(b,68+20*i);});
    c.names.forEach((s,i)=>{const x=utf8(s,64,true);b.writeUInt32LE(x.length,344+68*i);x.copy(b,348+68*i);});return b;
  }
  function wire(b,size){if(!Buffer.isBuffer(b)||b.length!==size||b.readUInt32LE(0)!==size||b.readUInt32LE(4)!==1)throw new TypeError('invalid copied TimeField wire');return Buffer.from(b);}
  function text(b,at,max,empty){const n=b.readUInt32LE(at);if(n>max||(!n&&!empty))throw new TypeError('invalid TimeField copied length');const bytes=b.subarray(at+4,at+4+n);const s=bytes.toString('utf8');if(!utf8(s,max,empty).equals(bytes)||b.subarray(at+4+n,at+4+max).some(v=>v!==0))throw new TypeError('invalid copied UTF-8/padding');return s;}
  function decodeConfig(raw){const b=wire(raw,616),flags=uint(b.readUInt32LE(8),7),bounds=uint(b.readUInt32LE(20),3);
    if(b.readUInt32LE(24)||b.readUInt32LE(28))throw new TypeError('nonzero TimeField reserved');
    const readTime=at=>[b.readUInt32LE(at),b.readUInt32LE(at+4),b.readUInt32LE(at+8)];
    if((!(bounds&1)&&readTime(32).some(Boolean))||(!(bounds&2)&&readTime(44).some(Boolean)))throw new TypeError('nonzero inactive TimeField bounds');
    const tokens=Array.from({length:14},(_,i)=>text(b,64+20*i,16,false));
    return localized_time_config({disabled:!!(flags&1),readonly:!!(flags&2),ownerInvalid:!!(flags&4),cycle:choice(b.readUInt32LE(12),[0,1])?'h12':'h23',precision:choice(b.readUInt32LE(16),[0,1])?'second':'minute',minimum:bounds&1?readTime(32):null,maximum:bounds&2?readTime(44):null,editGeneration:b.readBigUInt64LE(56),locale:{digits:tokens.slice(0,10),separator:tokens[10],periodSeparator:tokens[11],am:tokens[12],pm:tokens[13]},names:Array.from({length:4},(_,i)=>text(b,344+68*i,64,true))});
  }
  function call(app,op,node=null,value=null){app._alive();if(typeof native.timeFieldLocalized!=='function')throw new Error('localized TimeField addon unavailable');return native.timeFieldLocalized(app.handle,op,node,value,null);}
  function supports(app){app._alive();return typeof native.timeFieldLocalized==='function'&&call(app,0);}
  function enable(app,capacity=4096){uint(capacity,4096);call(app,1,null,capacity);app.localizedTimeEnabled=true;return app;}
  function observation(app,raw,result=false){
    const b=wire(raw,result?80:64);
    if(b.readUInt32LE(12)||(result&&b.readUInt32LE(8)))throw new TypeError('nonzero TimeField reserved');
    const handle=freeze({slot:uint(b.readUInt32LE(16)),generation:uint(b.readUInt32LE(20))});if(!handle.generation)throw new TypeError('invalid TimeField generation');
    const frame=big(b.readBigUInt64LE(24));const at=result?48:32;
    const readTime=p=>time([b.readUInt32LE(p),b.readUInt32LE(p+4),b.readUInt32LE(p+8)]);
    const previous=readTime(at),value=readTime(at+12),frameStatus=uint(b.readUInt32LE(result?76:8),10);
    if(result&&frameStatus!==0)throw new TypeError('failed frame is not a current TimeField result');
    const record={handle,frame,node:app.nodes.get(keyFor(handle)),previous,value,validation:uint(b.readUInt32LE(result?36:56),6),rejectedCount:uint(b.readUInt32LE(result?44:60)),frameStatus};
    if(result){record.status=uint(b.readUInt32LE(32),6);record.changed=!!uint(b.readUInt32LE(40),1);record.queuedChangeCount=uint(b.readUInt32LE(72),1);if(record.changed!==(seconds(previous)!==seconds(value)))throw new TypeError('incoherent TimeField result');}
    else if(seconds(previous)===seconds(value))throw new TypeError('net-zero TimeField change');
    return freeze(record);
  }
  function poll(app){const raw=call(app,6);if(raw===undefined)return undefined;const event=observation(app,raw);if(!(event.node instanceof LocalizedTimeField))throw new TypeError('unknown localized TimeField observation identity');event.node.timeValue=[...event.value];return event;}
  function dispatch(app){
    if(app.localizedTimeDispatching||app.sliderDispatching)throw new Error('nested localized TimeField dispatch unavailable');
    app.localizedTimeDispatching=true;let delivered=0;
    try{while(!app.closed&&!app.disposed){const event=poll(app);if(event===undefined)break;const callback=event.node?.localizedChangeValue;if(callback)callback(event);delivered++;}}
    finally{app.localizedTimeDispatching=false;}return delivered;
  }
  class LocalizedTimeField extends Node {
    constructor(key,config={},text='Time'){super(55,text);this.key(key);this.localizedConfigValue=localized_time_config(config);this.localizedChangeValue=null;}
    configure(config){const c=localized_time_config(config);if(this.owner)call(this.owner,2,this[NODE],encode(c));this.localizedConfigValue=c;return this;}
    get_config(){return this.owner?decodeConfig(call(this.owner,3,this[NODE])):this.localizedConfigValue;}
    latest_result(){if(!this.owner)throw new Error('localized TimeField is not mounted');const record=observation(this.owner,call(this.owner,4,this[NODE]),true);if(record.node!==this)throw new TypeError('mismatched TimeField result identity');return record;}
    clear_validation(){if(!this.owner)throw new Error('localized TimeField is not mounted');call(this.owner,5,this[NODE]);return this;}
    disabled(v=true){bool(v);return this.configure({...this.get_config(),disabled:v});}
    readonly(v=true){bool(v);return this.configure({...this.get_config(),readonly:v});}
    owner_invalid(v=true){bool(v);return this.configure({...this.get_config(),ownerInvalid:v});}
    on_localized_change(callback){if(callback!==null&&typeof callback!=='function')throw new TypeError('expected TimeField callback or null');this.localizedChangeValue=callback;return this;}
    on_change(){throw new TypeError('localized TimeField uses applied on_localized_change records');}
  }
  function localized_time_field(key,config={},text='Time'){return new LocalizedTimeField(key,config,text);}
  function preflight(app,root){let found=false;const walk=(node,ancestors)=>{if(node instanceof LocalizedTimeField){if(ancestors.some(k=>k!==1&&k!==2))throw new TypeError('localized TimeField requires Row/Column ancestry');found=true;}for(const child of node.childrenValue)walk(child,[...ancestors,node.kind]);};walk(root,[]);if(found){if(!supports(app))throw new Error('localized TimeField V1 unavailable');if(!app.localizedTimeEnabled)enable(app);}}
  function mount(app,node){if(node instanceof LocalizedTimeField)call(app,2,node[NODE],encode(node.localizedConfigValue));}
  function release(node){if(node instanceof LocalizedTimeField)node.localizedChangeValue=null;}
  return {exports:{LocalizedTimeField,localized_time_field,localized_time_config,time_locale_english,time_locale_arabic},preflight,mount,release,supports,enable,poll,dispatch};
};
