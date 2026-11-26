'use strict';
const {App,attachment,column}=require('..');
const app=new App({backend:'headless',width:480,height:400});
try {
  const card=attachment('report','Quarterly report.pdf').detail('2.4 MB').visual_size(280,0);
  if(app.supports_attachment_options())card.design_size('lg').attachment_options({vertical:true,media:true,mediaAspect:2,title:{fontSize:18,weight:2}});
  if(app.supports_attachment_preview())card.attachment_preview_rgba(new Uint8Array([36,92,182,255,36,92,182,255]),2,1);
  app.mount(column().children(card)).step();
  if(card.bounds().width!==280)throw new Error('missing retained Attachment width');
}finally{app.dispose();}
