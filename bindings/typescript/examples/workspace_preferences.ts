import {App, accordion, column, item, label} from '../../node/index.cjs';
import type {DisclosureConfig, DisclosureTriggerStyle} from '../../node/index.cjs';

const config: DisclosureConfig = {
  width: 360, designSize: 'lg', joined: true, bordered: true,
  single: true, allowCollapse: true, collapsedIcon: 'plus', expandedIcon: 'minus',
};
const style: DisclosureTriggerStyle = {hoverRgba: 0x4879e02b, cursor: 'pointer'};
const app = new App({width: 480, height: 320});
try {
  let expanded = false;
  const status = label('Open a section to edit your preferences.');
  const account = item('account', 'Account').children(label('Personal details'));
  const editor = item('editor', 'Editor').children(label('Auto-save is enabled.')).on_change(event => {
    expanded = event.checked;
    status.label(expanded ? 'Editing workspace preferences.' : 'Editor preferences closed.');
  });
  const sections = accordion('sections').children(account, editor);
  if (app.supports_disclosure_presentation()) {
    sections.disclosure_configure(config);
    editor.set_trigger_style(style);
  }
  if (app.supports_disclosure_title_state()) {
    sections.set_disclosure_title_state('hovered', {titleRgba: 0x275dccff});
    editor.set_disclosure_title_state('focus-visible', {typography: {fontSize: 17, weight: 2}});
  }
  if (app.supports_disclosure_heading()) {
    sections.set_disclosure_heading('normal', {
      box: {width: -1, height: 44, padding: 12, gap: 8},
      typography: {fontSize: 16, weight: 2}, titleRgba: 0x23354aff,
    });
    editor.set_disclosure_heading('expanded', {titleRgba: 0x275dccff});
  }
  if (app.supports_disclosure_divider()) {
    sections.set_disclosure_divider({thickness: 1, insetLeft: 12, insetRight: 12, colorRgba: 0xc8d0daff});
  }
  app.mount(column().key('workspace').gap(12).children(label('Workspace preferences'), sections, status)).step();
  const {x, y} = editor.bounds();
  app.pointer('down', x + 20, y + 12).pointer('up', x + 20, y + 12).step();
  if (!expanded) throw new Error('Editor preferences did not open');
} finally {
  app.dispose();
}
