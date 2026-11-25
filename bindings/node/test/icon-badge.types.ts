// SPDX-License-Identifier: MIT
import {App, IconSource, configured_badge, configured_icon, icon_rotation, button} from '../../typescript/index';
import type {BadgeConfig, IconOptions, RetainedIconResult} from '../../typescript/index';

const app = new App();
const resource = app.icon_resource(IconSource.named('check'));
const artwork = configured_icon('icon', resource, {style: {corners: [1, 2, 3, 0], opacity: 0},
  constraints: {maxWidth: 0}, transform: icon_rotation(.3)});
const status = configured_badge('status', {variant: 'icon', artwork: resource,
  artworkOptions: {decorative: true}}).children(button('child')).disabled(false);
status.configure({...status.get_config(), count: 9007199254740993n});
const config: BadgeConfig = status.get_config();
const options: IconOptions | undefined = artwork.get_config()?.options;
const result: RetainedIconResult | undefined = artwork.result();
const sequence: bigint | undefined = result?.frameSequence;
void [sequence, config, options];
// @ts-expect-error explicit zero is a number
artwork.configure(resource, {width: '0'});
// @ts-expect-error icon source must be exclusive
configured_badge('invalid', {variant: 'icon', artwork: resource, namedIcon: 'check'});
// @ts-expect-error count variants do not have artwork
status.configure({count: 1n, artwork: resource});
// @ts-expect-error indicator bounds are snapshots
status.indicator_bounds()!.x = 0;
// @ts-expect-error configured badges do not emit legacy change callbacks
status.on_change(() => undefined);
app.dispose();
