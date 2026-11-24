import {App, ResponsiveSkeleton, column, responsive_skeleton, skeleton} from '../index.cjs';
import type {SkeletonConfig, SkeletonPresentation} from '../index.cjs';

const app = new App();
const line = responsive_skeleton('line', {width: .5, backgroundRgba: 0});
const circle = new ResponsiveSkeleton('circle', {widthMode: 'fixed', width: 40, height: 40});
const config: SkeletonConfig = line.get_config();
line.configure({...config, reducedMotion: true});
const sample: SkeletonPresentation | undefined = line.presentation();
const frame: bigint | undefined = sample?.frameSequence;
app.mount(column().children(line, circle, skeleton('legacy').visual_size(72, 12)));
app.supports_skeleton_presentation();
void frame;
if (sample) {
  // @ts-expect-error presentation fields are read-only
  sample.opacity = 2;
}
// @ts-expect-error use responsive config to size the modern route
line.visual_size(40, 40);
// @ts-expect-error supported width vocabulary
circle.configure({widthMode: 'percent'});
// @ts-expect-error legacy placeholder has no responsive sample
skeleton('legacy').presentation();
