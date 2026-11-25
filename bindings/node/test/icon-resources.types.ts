// SPDX-License-Identifier: MIT
import {
  App, IconAvailability, IconBitmap, IconError, IconErrorValue, IconResource, IconSource,
} from '../../typescript/index';

const app = new App();
const resource: IconResource = app.icon_resource(IconSource.path('tiles/check.svg'),
  {maxSvgBytes: 4096});
resource.put_asset('tiles/check.svg', new Uint8Array([0x3c, 0x73, 0x76, 0x67, 0x3e]));
const error: IconErrorValue = resource.prepare(24, 24, [1, 0, 0, 1, 0, 0]);
if (error === IconError.OK) {
  const bitmap: IconBitmap = resource.bitmap();
  const revision: bigint = bitmap.revision;
  const alpha: number = bitmap.rgba[3];
  void [revision, alpha];
}
const revision: bigint = resource.info().sourceRevision;
const defaultColor = app.query_icon('check') === IconAvailability.EMBEDDED;
void [revision, defaultColor];

// @ts-expect-error the six coefficients have a fixed order and length
resource.prepare(24, 24, [1, 1]);
// @ts-expect-error paths only accept string keys
IconSource.path(new Uint8Array());
// @ts-expect-error resource information is immutable
resource.info().references = 4;
// @ts-expect-error use a supported source kind
app.icon_resource({kind: 'file', text: 'check.svg'});
// @ts-expect-error this type is an opaque app-owned resource
const fabricated: IconResource = {};
void fabricated;
resource.close();
app.dispose();
