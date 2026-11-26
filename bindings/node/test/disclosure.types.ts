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
import type {DisclosureHeading,DisclosureDivider,DisclosureHeadingLayer} from '../index.cjs';
const heading:DisclosureHeading={box:{width:-1,height:44},typography:{fontSize:16,weight:2,wrap:true},titleRgba:0};
const divider:DisclosureDivider={thickness:0,colorRgba:0};
const layer:DisclosureHeadingLayer='expanded';
accordion().set_disclosure_heading(layer,heading).set_disclosure_divider(divider).set_disclosure_divider(null);
collapsible().set_disclosure_heading('normal',null);
item().set_disclosure_heading('disabled',heading);
// @ts-expect-error invalid heading layer
item().set_disclosure_heading('hover',heading);
// @ts-expect-error invalid typography weight
item().set_disclosure_heading('normal',{typography:{weight:4}});
// @ts-expect-error divider requires Accordion, not Item
item().set_disclosure_divider(divider);

// @ts-expect-error fontSize is required when typography is supplied
item().set_disclosure_heading('normal', {typography: {weight: 2}});
