# /// script
# dependencies = ["wasmtime==49.0.0"]
# ///
"""Execute actual composed CUI Input/provider tests in the linked WASI reactor."""
import argparse
from pathlib import Path
import wasmtime
p=argparse.ArgumentParser(description=__doc__);p.add_argument('module',type=Path);a=p.parse_args()
engine=wasmtime.Engine();module=wasmtime.Module.from_file(engine,str(a.module));store=wasmtime.Store(engine)
wasi=wasmtime.WasiConfig();wasi.inherit_stdout();wasi.inherit_stderr();store.set_wasi(wasi)
linker=wasmtime.Linker(engine);linker.define_wasi();instance=linker.instantiate(store,module);exports=instance.exports(store)
assert '_initialize' in exports and '_start' not in exports
exports['_initialize'](store);assert exports['regex_probe_run'](store)==0
print('PASS Wasmtime49: actual CUI Input regex typing/clipboard/Undo/IME/masking/resource tests in initialized reactor')
print('imports:',[(x.module,x.name) for x in module.imports])
