import {App, attachment, column} from '../../node/index.cjs';
import type {AttachmentOptions, AttachmentTextStyle} from '../../node/index.cjs';
const title: AttachmentTextStyle = {fontSize: 18, weight: 2};
const options: AttachmentOptions = {vertical: true, media: true, mediaAspect: 2, title};
const app = new App({backend: 'headless', width: 480, height: 400});
try {
  const card = attachment('report', 'Quarterly report.pdf').detail('2.4 MB').visual_size(280, 0);
  if (app.supports_attachment_options()) card.design_size('lg').attachment_options(options);
  if (app.supports_attachment_preview()) card.attachment_preview_rgba(new Uint8Array([36,92,182,255]), 1, 1);
  app.mount(column().children(card)).step();
} finally { app.dispose(); }
