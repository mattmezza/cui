// SPDX-License-Identifier: MIT
import {App, row, column, label, button} from '../../node/index.cjs';
const count = label('Changes: 0');
let changes: number = 0;
const save = button('save').primary().rounded_md().label('Save changes')
  .on_click(() => count.label(`Changes: ${++changes}`));
const app = new App({backend: 'headless', width: 480, height: 240});
app.mount(column().gap_units(2).children(row().children(save, count)));
app.step();
const bounds = save.bounds();
app.pointer('move', bounds.x + 2, bounds.y + 2)
  .pointer('down', bounds.x + 2, bounds.y + 2)
  .pointer('up', bounds.x + 2, bounds.y + 2);
app.step();
if (changes !== 1) throw new Error(`expected one click, got ${changes}`);
app.dispose();
console.log('TypeScript native CUI click passed');
