import {App, button, column, label, responsive_skeleton} from '../../node/index.cjs';
import type {SkeletonConfigInput, SkeletonPresentation} from '../../node/index.cjs';

const avatarStyle: SkeletonConfigInput = {
  widthMode: 'fixed', width: 40, height: 40, radius: 20,
};
const avatar = responsive_skeleton('avatar', avatarStyle);
const name = responsive_skeleton('name', {width: .65, radius: 6});
const role = responsive_skeleton('role', {width: .4, height: 12, secondary: true});
const description = responsive_skeleton('description', {height: 36, radius: 8});
const placeholders = [avatar, name, role, description];
const title = label('Workspace profile');
const status = label('Loading profile…');
const finish = button('finish').primary().label('Finish loading');
let loaded = false;

finish.on_click(() => {
  for (const placeholder of placeholders) placeholder.remove();
  title.label('Alex Morgan');
  status.label('Workspace admin · alex@example.com');
  finish.disabled().label('Profile loaded');
  loaded = true;
});

const app = new App({backend: 'headless', width: 480, height: 320});
try {
  app.mount(column().gap(10).children(title, placeholders, status, finish));
  app.step(.25);
  const sample: SkeletonPresentation | undefined = name.presentation();
  if (!sample || sample.frameSequence < 1n || sample.nextWakeSeconds <= sample.coreTime)
    throw new Error('Expected a live Skeleton sample');
  if (avatar.bounds().width !== 40 || Math.abs(name.bounds().width - 448 * .65) > .001 ||
      description.bounds().width !== 448)
    throw new Error('Expected responsive placeholder geometry');

  const before = status.bounds().y;
  const {x, y, width, height} = finish.bounds();
  app.pointer('down', x + width / 2, y + height / 2)
    .pointer('up', x + width / 2, y + height / 2).step();
  app.step();
  if (!loaded || status.bounds().y >= before) throw new Error('Profile did not replace placeholders');
  console.log('TypeScript Skeleton profile: loaded and placeholders released');
} finally {
  app.dispose();
}
