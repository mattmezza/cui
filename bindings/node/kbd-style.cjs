'use strict';

const dimensions = [
  ['maxWidth', 0, 32, 4096, true],
  ['radius', 4, 44, 4096, false],
  ['fontSize', 7, 52, 512, true],
  ['lineHeight', 8, 56, 4096, false],
];
const flags = [['fill', 1], ['wrap', 2], ['italic', 10], ['monospace', 11]];
const colors = [['background', 5, 16], ['textColor', 12, 24]];
const otherKeys = ['paddingX', 'paddingY', 'borderWidth', 'borderColor', 'weight', 'textGradient'];
const allowed = new Set([...dimensions, ...flags, ...colors].map(([key]) => key).concat(otherKeys));

function number(value, maximum = 4096, positive = false) {
  if (typeof value !== 'number' || !Number.isFinite(value) || value < 0 || value > maximum ||
      (positive && Math.fround(value) === 0)) throw new TypeError('invalid Kbd style number');
  return value;
}

function color(value) {
  if (!Number.isInteger(value) || value < 0 || value > 0xffffffff)
    throw new TypeError('Kbd color must be numeric RGBA');
  return value;
}

function writeGradient(wire, gradient) {
  if (!gradient || typeof gradient !== 'object' || Array.isArray(gradient) ||
      Object.keys(gradient).some(key => !['stops', 'x0', 'y0', 'x1', 'y1'].includes(key)))
    throw new TypeError('invalid Kbd gradient');
  const stops = gradient.stops;
  if (!Array.isArray(stops) || stops.length < 2 || stops.length > 8)
    throw new TypeError('Kbd gradient requires 2..8 stops');
  let previous = 0;
  for (const [index, stop] of stops.entries()) {
    if (!stop || typeof stop !== 'object' || Object.keys(stop).length !== 2 ||
        !Object.hasOwn(stop, 'position') || !Object.hasOwn(stop, 'color'))
      throw new TypeError('invalid Kbd gradient stop');
    const position = number(stop.position, 1);
    if (position < previous) throw new TypeError('Kbd stops must be ordered');
    previous = position;
    wire.writeFloatLE(position, 80 + index * 8);
    wire.writeUInt32LE(color(stop.color), 84 + index * 8);
  }
  const {x0 = 0, y0 = 0, x1 = 1, y1 = 0} = gradient;
  const ends = [x0, y0, x1, y1].map(value => Math.fround(number(value, 1)));
  if (ends[0] === ends[2] && ends[1] === ends[3])
    throw new TypeError('Kbd gradient endpoints must differ');
  ends.forEach((value, index) => wire.writeFloatLE(value, 64 + index * 4));
  wire.writeUInt32LE(stops.length, 60);
}

// Copy the authoring options into the private, little-endian addon wire record.
module.exports = function kbdStyleWire(style) {
  if (style === null) return null;
  if (!style || typeof style !== 'object' || Array.isArray(style) ||
      Object.keys(style).some(key => !allowed.has(key))) throw new TypeError('invalid Kbd style');

  const wire = Buffer.alloc(144);
  wire.writeUInt32LE(wire.length, 0);
  wire.writeUInt32LE(1, 4);
  let fields = 0;
  for (const [key, bit, offset, maximum, positive] of dimensions) {
    if (!Object.hasOwn(style, key)) continue;
    wire.writeFloatLE(number(style[key], maximum, positive), offset);
    fields |= 1 << bit;
  }
  for (const [key, bit] of flags) {
    if (!Object.hasOwn(style, key)) continue;
    const value = style[key];
    if (typeof value !== 'boolean') throw new TypeError('Kbd flags must be boolean');
    if (value) fields |= 1 << bit;
  }
  for (const [key, bit, offset] of colors) {
    if (!Object.hasOwn(style, key)) continue;
    wire.writeUInt32LE(color(style[key]), offset);
    fields |= 1 << bit;
  }
  if (Object.hasOwn(style, 'paddingX') || Object.hasOwn(style, 'paddingY')) {
    const {paddingX = 4, paddingY = 2} = style;
    wire.writeFloatLE(number(paddingX), 36);
    wire.writeFloatLE(number(paddingY), 40);
    fields |= 1 << 3;
  }
  const hasBorderWidth = Object.hasOwn(style, 'borderWidth');
  const hasBorderColor = Object.hasOwn(style, 'borderColor');
  if (hasBorderWidth !== hasBorderColor) throw new TypeError('Kbd border requires width and color');
  if (hasBorderWidth) {
    wire.writeFloatLE(number(style.borderWidth), 48);
    wire.writeUInt32LE(color(style.borderColor), 20);
    fields |= 1 << 6;
  }
  if (Object.hasOwn(style, 'weight')) {
    const weight = ['normal', 'medium', 'semibold', 'bold'].indexOf(style.weight);
    if (weight < 0) throw new TypeError('invalid Kbd weight');
    wire.writeUInt32LE(weight, 28);
    fields |= 1 << 9;
  }
  if (Object.hasOwn(style, 'textGradient')) {
    if (Object.hasOwn(style, 'textColor')) throw new TypeError('Kbd text color and gradient are exclusive');
    writeGradient(wire, style.textGradient);
    fields |= 1 << 13;
  }
  wire.writeUInt32LE(fields, 8);
  return fields ? wire : null;
};
