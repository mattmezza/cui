import {App,attachment,label} from '../index.cjs';
import type {AttachmentOptions,AttachmentTextStyle} from '../index.cjs';
const title: AttachmentTextStyle={fontSize:20,weight:2,colorRgba:0};
const options: AttachmentOptions={vertical:true,media:true,mediaAspect:2,title,titleStatus:'complete'};
const card=attachment().attachment_options(options).design_size('lg').attachment_preview_rgba(new Uint8Array(4),1,1).clear_attachment_preview().attachment_options(null);
const app=new App();const a:boolean=app.supports_attachment_options(), b:boolean=app.supports_attachment_preview();void card;void a;void b;
// @ts-expect-error labels do not have Attachment options
label().attachment_options(options);
// @ts-expect-error weight is a bounded enum
attachment().attachment_options({title:{weight:9}});
// @ts-expect-error decoded pixels require bytes
attachment().attachment_preview_rgba([1,2,3,4],1,1);
// @ts-expect-error status uses supported names
attachment().attachment_options({titleStatus:5});
// @ts-expect-error unknown status names are rejected
attachment().attachment_options({descriptionStatus:"done"});
