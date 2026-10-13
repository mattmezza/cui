// SPDX-License-Identifier: MIT
// No package dependencies: minimal declarations for this example's Node globals.
declare function require(path: string): unknown;
declare const process: { argv: string[] };
interface Host { run(callback: (count: number) => void, font?: string, frames?: number): number }
const host = require('../build/cui.node') as Host;
let observed = 0;
const callback = (count: number): void => { observed = count; console.log(`TypeScript count: ${count}`); };
const font = process.argv[2];
const status = font ? host.run(callback, font, Number(process.argv[3] || 0)) : host.run(callback);
if (status !== 0 || (!font && observed !== 2)) throw new Error(`CUI failed: ${status}, count ${observed}`);
