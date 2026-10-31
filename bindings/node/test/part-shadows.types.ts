import {checkbox,switch_control,slider,radio_group,radio_item,item,ShadowPart as P,BoxShadow} from '../../typescript';
checkbox().part_shadows(P.INDICATOR,[new BoxShadow()]).part_shadows(P.ROOT,[]);
switch_control().part_shadows(P.TRACK,[]).part_shadows(P.THUMB,[]);
slider().part_shadows(P.MAIN,[]);
radio_group().part_shadows(P.ROOT,[]);
radio_item().part_shadows(P.INDICATOR,[]);
// @ts-expect-error Slider has no thumb box
slider().part_shadows(P.THUMB,[]);
// @ts-expect-error Group has no one main indicator
radio_group().part_shadows(P.MAIN,[]);
// @ts-expect-error Generic ITEM makes no indicator promise
item().part_shadows(P.INDICATOR,[]);
