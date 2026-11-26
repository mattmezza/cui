import {accordion,collapsible,item} from '../index.cjs';
import type {DisclosureConfig,DisclosureTriggerStyle} from '../index.cjs';
const config:DisclosureConfig={joined:true,width:-1,cardBackgroundRgba:0,collapsedIcon:'chevron-right'};
const style:DisclosureTriggerStyle={hoverRgba:0,cursor:'zoom_out'};
accordion('workspace').disclosure_configure(config).disclosure_configure(null);
collapsible('advanced').disclosure_configure({borderless:true}).set_trigger_style(style).set_trigger_style(null);
item('editor').set_trigger_style(style);
// @ts-expect-error invalid cursor
item().set_trigger_style({cursor:'invalid'});
// @ts-expect-error flags are boolean
accordion().disclosure_configure({joined:1});
