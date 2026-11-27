import {App, RotatingSpinner, rotating_spinner, spinner, column} from '../index.cjs';
import type {SpinnerConfig, SpinnerPresentation} from '../index.cjs';
const app = new App();
const indicator = rotating_spinner('loading', {size:'medium', easing:'quadratic'});
const custom = new RotatingSpinner('custom', {size:'custom',pixels:24,rgba:0});
const config: SpinnerConfig = indicator.get_config();
indicator.configure({...config,reducedMotion:true});
const sample: SpinnerPresentation | undefined = indicator.presentation();
const frame: bigint | undefined = sample?.frameSequence;
app.mount(column().children(indicator,custom,spinner('legacy').visual_size(20,20)));
app.supports_spinner_presentation();
void frame;
// @ts-expect-error configuration is immutable
config.icon = 'loader';
if (sample) {
  // @ts-expect-error presentation is immutable
  sample.phase = .5;
}
// @ts-expect-error named size vocabulary
indicator.configure({size:'huge'});
// @ts-expect-error easing vocabulary
indicator.configure({easing:'bounce'});
// @ts-expect-error modern size is configured
indicator.visual_size(20,20);
// @ts-expect-error legacy Spinner has no copied presentation
spinner('legacy').presentation();
