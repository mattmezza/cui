import {App,BoxShadow,ShadowStyle,ShadowKind,ShadowPreset,CornerRadii,button,column,label,row,input,badge,checkbox,hover_card} from '../../typescript';
const radii=new CornerRadii(1,2,3,4);
const layer=BoxShadow.css(0,2,8,-1,0x10203040,ShadowKind.INSET);
const style=ShadowStyle.preset(ShadowPreset.MD,radii);
const b=button('b').label('Go').box_shadows([layer],radii).shadow_preset(ShadowPreset.SM,radii);
const root=column().box_shadows(style).children(row().box_shadows([]).children(label('text').box_shadows([])),b,input('i').box_shadows([]),badge('badge').box_shadows([]));
new App().mount(root);b.reset_box_shadows();
// @ts-expect-error Unsupported compound roots do not acquire the style surface through Label.
checkbox('no').box_shadows([]);
// @ts-expect-error Portals require a separate approved addressing contract.
hover_card('no').shadow_preset(ShadowPreset.MD);
// @ts-expect-error Typed kind is DROP or INSET.
new BoxShadow(0,0,1,0,2);
