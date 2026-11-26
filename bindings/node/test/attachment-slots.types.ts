import {App, attachment, attachment_slot, column, button, AttachmentSlot} from '..';
const slot: AttachmentSlot = attachment_slot('actions','actions',120).gap(4).disabled(false).children(button('Save').on_click(e => e.node.primary()));
const app = new App();
if (app.supports_attachment_slots()) app.mount(column().children(attachment('file','Report').children(slot)));
// @ts-expect-error invalid role
attachment_slot('bad','footer');
// @ts-expect-error invalid width type
attachment_slot('bad','actions','100');
// @ts-expect-error Actions require a preferred width
attachment_slot('bad', 'actions');
// @ts-expect-error Only Actions accept a positive width
attachment_slot('bad', 'overlay', 100);
