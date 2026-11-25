#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["playwright==1.58.0"]
# ///
"""Real WASM Table layout, appearance, selection and sorting."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
import threading
from playwright.sync_api import sync_playwright
from test_touch_components import Audit
PAINT='''()=>{const e=cuiGallery.exports,d=new TextDecoder();return Array.from({length:e.gallery_count()},(_,i)=>({type:e.gallery_command(i,0),b:[1,2,3,4].map(f=>e.gallery_command(i,f)),text:d.decode(new Uint8Array(e.memory.buffer,e.gallery_command_text(i),e.gallery_command_length(i)))}));}'''


def cell_pixel(audit, target):
    return audit.page.evaluate('''([x,y,w,h])=>{
        const canvas=document.querySelector('#preview');
        const scale=canvas.width/canvas.clientWidth;
        return Array.from(canvas.getContext('2d').getImageData(
            Math.floor((x+w*.85)*scale),Math.floor((y+h*.5)*scale),1,1).data);
    }''', target['f'][1:5])


def first_column(audit, role, row):
    return audit.target(role=role, predicate=lambda target: target['f'][22:24] == [row, 1])


def check_table_appearance(audit):
    audit.route('Table')
    assert audit.page.locator('#table-custom-header').is_visible()
    assert audit.page.locator('#table-striped').is_visible()
    surfaces = []
    for scheme in ('light', 'dark'):
        audit.page.emulate_media(color_scheme=scheme)
        audit.configure('table-custom-header', True)
        audit.configure('table-striped', True)
        header = audit.target('Name', 29)
        first, second, third = [first_column(audit, 28, row) for row in (2, 3, 4)]
        header_color = cell_pixel(audit, header)
        base_color = cell_pixel(audit, first)
        surfaces.append(base_color)
        stripe_color = cell_pixel(audit, second)
        assert stripe_color != base_color and cell_pixel(audit, third) == base_color
        audit.configure('table-custom-header', False)
        assert cell_pixel(audit, header) != header_color
        assert cell_pixel(audit, second) == stripe_color
        audit.configure('table-striped', False)
        assert cell_pixel(audit, second) == base_color
    assert surfaces[0] != surfaces[1]


def check_data_table_appearance(audit):
    audit.route('DataTable')
    assert not audit.page.locator('#table-custom-header').is_visible()
    assert audit.page.locator('#table-striped').is_visible()
    for scheme in ('light', 'dark'):
        audit.page.emulate_media(color_scheme=scheme)
        audit.configure('table-striped', True)
        first, second = [first_column(audit, 30, row) for row in (1, 2)]
        base_color, stripe_color = cell_pixel(audit, first), cell_pixel(audit, second)
        assert base_color != stripe_color
        audit.configure('table-striped', False)
        assert cell_pixel(audit, second) == base_color
        audit.configure('table-striped', True)
        assert cell_pixel(audit, second) == stripe_color

    audit.tap(second)
    assert audit.value(second, 11) == 1
    primary = audit.page.evaluate('''() => [0,1,2,3].map(
        channel => cuiGallery.exports.gallery_theme_channel(2, channel))''')
    assert cell_pixel(audit, second) == primary and cell_pixel(audit, first) == base_color

    header = audit.target('Name', 29)
    header_y = header['f'][2]
    header_color = cell_pixel(audit, header)
    audit.tap(header)
    sorted_second = first_column(audit, 30, 2)
    assert sorted_second['id'] != second['id']
    assert cell_pixel(audit, sorted_second) == stripe_color
    audit.tap(audit.target('Name', 29))
    assert first_column(audit, 30, 2)['id'] == second['id']
    assert audit.value(second, 11) == 1 and cell_pixel(audit, second) == primary

    point = audit.point(first_column(audit, 30, 3))
    body = audit.target('Rows', 34)
    audit.page.mouse.move(point['x'], point['y'])
    audit.page.mouse.wheel(0, 32)
    audit.page.wait_for_function(
        '''id => {
            const api=cuiGallery.exports;
            for (let i=0;i<api.gallery_semantic_count();++i)
                if (String(api.gallery_semantic_id(i))===id)
                    return Math.abs(api.gallery_semantic(i,19)-32)<.01;
            return false;
        }''', arg=body['id'])
    header = audit.target('Name', 29)
    assert header['f'][2] == header_y and cell_pixel(audit, header) == header_color
    selected = first_column(audit, 30, 2)
    assert selected['f'][2] == second['f'][2] - 32
    assert selected['f'][11] == 1 and cell_pixel(audit, selected) == primary
    assert cell_pixel(audit, first_column(audit, 30, 3)) == base_color
    assert cell_pixel(audit, first_column(audit, 30, 4)) == stripe_color


def check_live_data_table(audit):
    def visible_cells():
        body = audit.target('Rows', 34)
        top, bottom = body['f'][2], body['f'][2] + body['f'][4]
        cells = [cell for cell in audit.snapshot() if cell['f'][0] == 30 and cell['f'][23] == 1
                 and cell['f'][2] < bottom and cell['f'][2] + cell['f'][4] > top]
        return body, sorted(cells, key=lambda cell: cell['f'][22])

    audit.configure('table-mode', 4)
    assert 'cui_table_restore_row_anchor' in audit.page.locator('#snippet').text_content()
    assert audit.target('Workspace files', 31)['f'][24] == 1000
    audit.drag(audit.target('Rows', 34), 0, -44, rx=.15, ry=.6)
    body, cells = visible_cells()
    first, selected, successor = cells[:3]
    assert abs(body['f'][19] - 44) < .2
    assert first['f'][22] == 2

    audit.tap(audit.target('Receive a file', 1))
    assert audit.target('File arriving…', 1)['f'][5]
    audit.tap(selected)
    assert audit.value(selected, 11) == 1 and audit.value(selected, 7) == 1
    audit.page.wait_for_function('''() => {
        const e = cuiGallery.exports;
        return Array.from({length:e.gallery_semantic_count()}, (_,i)=>i)
            .some(i=>e.gallery_semantic(i,0)===31 && e.gallery_semantic(i,24)===1001);
    }''')
    updated, cells = visible_cells()
    assert cells[0]['id'] == first['id'] and cells[0]['f'][22] == first['f'][22] + 1
    assert abs(cells[0]['f'][2] - first['f'][2]) < .2
    assert abs(updated['f'][19] - body['f'][19] - 32) < .2
    assert audit.value(selected, 11) == 1 and audit.value(selected, 7) == 1
    audit.page.keyboard.press('ArrowDown')
    audit.page.wait_for_function('''id => {
        const e = cuiGallery.exports;
        return Array.from({length:e.gallery_semantic_count()}, (_,i)=>i)
            .some(i=>String(e.gallery_semantic_id(i))===id && e.gallery_semantic(i,11));
    }''', arg=successor['id'])
    assert audit.value(successor, 7) == 1

    audit.page.set_viewport_size({'width':390, 'height':844})
    audit.configure('disabled', True)
    assert audit.target('Receive a file', 1)['f'][5]
    audit.tap(audit.target('Receive a file', 1))
    assert audit.page.evaluate('cuiGallery.exports.gallery_next_wake()') == 0
    audit.configure('disabled', False)
    audit.tap(audit.target('Receive a file', 1))
    assert audit.target('File arriving…', 1)['f'][5]
    audit.configure('table-mode', 0)
    assert audit.page.evaluate('cuiGallery.exports.gallery_next_wake()') == 0
    audit.configure('table-mode', 4)
    assert audit.target('Workspace files', 31)['f'][24] == 1000
    audit.target('Receive a file', 1)


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--directory',type=Path,default=Path('build/web'));p.add_argument('--chromium');args=p.parse_args()
 class Handler(SimpleHTTPRequestHandler):
  def log_message(self,*args):pass
 server=ThreadingHTTPServer(('127.0.0.1',0),partial(Handler,directory=str(args.directory.resolve())));threading.Thread(target=server.serve_forever,daemon=True).start()
 try:
  with sync_playwright()as pw:
   browser=pw.chromium.launch(headless=True,**({'executable_path':args.chromium}if args.chromium else{}));a=Audit(browser,f'http://127.0.0.1:{server.server_port}')
   a.route('Table')
   heights=[0,20,24,32,32,40,48,56,64,80,104,128,152,200,264]
   for density in [0,1]:
    a.configure('density',density)
    for size in range(1,15):
     a.configure('design-size',size)
     root=a.target('Recent workspace files',26);assert root['f'][24:26]==[4,3]
     header=a.target('Name',29);assert header['f'][22:24]==[1,1]and header['f'][26]
     assert abs(header['f'][4]-heights[size]*(.875 if density else 1))<1,(density,size,header)
     cells=[s for s in a.snapshot()if s['f'][0]==28];assert len(cells)==9
     assert all(s['name']!='Cell'and s['f'][22]>1 and s['f'][23]>0 for s in cells)
     paints=a.page.evaluate(PAINT)
     text=next(c for c in paints if c['text']=='Name')
     assert text['b'][1]>=header['f'][2]and text['b'][1]+text['b'][3]<=header['f'][2]+header['f'][4]+.5
     assert a.page.evaluate('cuiGallery.exports.gallery_error()')==0
   a.configure('density',0);a.route('DataTable');a.configure('table-selection',2)
   a.target('Workspace files',31)
   header=a.target('Name',29)
   for sort in [2,1,0]:
    a.tap(a.target('Name',29));assert a.value(header,8)==sort
   # Selection opt-out retains base action behavior and rejects row/cell selection.
   a.tap(a.target('Actions off',1));a.tap(a.target('Select on',1))
   before=[s['id']for s in a.snapshot()if s['f'][0]==30 and s['f'][11]]
   a.tap(a.target('Inspect',1))
   assert any('inspections: 1'in t for t in a.texts())
   after=[s['id']for s in a.snapshot()if s['f'][0]==30 and s['f'][11]];assert after==before
   a.tap(a.target('Select off',1))
   cell=a.target('Ready',30);assert cell['f'][26]and not cell['f'][5]
   a.tap(cell);assert a.value(cell,11)==0
   a.tap(a.target('Status',29));assert a.target('Status',29)['f'][8]==2
   a.page.set_viewport_size({'width':390,'height':1200});a.route('DataTable')
   a.tap(a.target('Inspect',1));assert any('inspections: 3'in t for t in a.texts()),a.texts()
   a.configure('disabled',True);assert a.target('Inspect',1)['f'][5]
   a.configure('disabled',False)
   a.route('Table');a.configure('design-size',14)
   owner=a.target(predicate=lambda s:s['f'][17]and s['f'][20]>40)
   before=a.value(owner,18);a.drag(owner,-80,0,rx=.5,ry=.5)
   assert a.value(owner,18)>before
   assert a.page.evaluate('cuiGallery.exports.gallery_error()')==0
   assert not a.errors,a.errors
   a.page.close()
   appearance = Audit(browser,f'http://127.0.0.1:{server.server_port}')
   check_table_appearance(appearance)
   check_data_table_appearance(appearance)
   check_live_data_table(appearance)
   assert appearance.page.evaluate('cuiGallery.exports.gallery_error()') == 0
   assert not appearance.errors, appearance.errors
   browser.close()
 finally:server.shutdown()
 print('Table selection contracts browser passed')
if __name__=='__main__':main()
