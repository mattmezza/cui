#!/usr/bin/env python3
"""Prerender shareable component/doc URLs with metadata available without JavaScript."""
import argparse
from html import escape, unescape
import json
from pathlib import Path
import re
import struct
from urllib.parse import unquote, urljoin, urlsplit
from xml.etree import ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SITE = 'https://matteo.merola.co/cui/'


def component_slug(name):
    return re.sub(r'[^a-z0-9]+', '-', name.lower()).strip('-')


def plain_text(markup):
    return re.sub(r'\s+', ' ', unescape(re.sub(r'<[^>]*>', ' ', markup))).strip()


def metadata(title, description, canonical, image, image_alt):
    values = [('description', description), ('og:title', title), ('og:type', 'website'),
              ('og:description', description), ('og:url', canonical), ('og:site_name', 'CUI'),
              ('og:locale', 'en_US'), ('og:image', image), ('og:image:secure_url', image),
              ('og:image:type', 'image/png'), ('og:image:width', '1200'),
              ('og:image:height', '630'), ('og:image:alt', image_alt),
              ('twitter:card', 'summary_large_image'), ('twitter:title', title),
              ('twitter:description', description), ('twitter:image', image),
              ('twitter:image:alt', image_alt)]
    tags = [f'<title>{escape(title)}</title>', f'<link rel="canonical" href="{escape(canonical, quote=True)}">']
    for name, value in values:
        attribute = 'property' if name.startswith('og:') else 'name'
        tags.append(f'<meta {attribute}="{name}" content="{escape(value, quote=True)}">')
    return '\n'.join(tags)


def build(output, site_url=DEFAULT_SITE):
    site = urlsplit(site_url)
    if site.scheme != 'https' or not site.netloc or site.query or site.fragment or not site.path.endswith('/'):
        raise ValueError('Site URL must be an absolute HTTPS directory without query/fragment')
    template = (output / 'index.html').read_text()
    # Reuse the resolved template; replace our prior block for deterministic rebuilds.
    template = re.sub(r'<!--cui-sharing:start-->.*?<!--cui-sharing:end-->',
                      '<title>CUI · Portable C11 UI</title>', template, flags=re.S)
    template = re.sub(r'<noscript><main>.*?</main></noscript>', '', template, flags=re.S)
    template = re.sub(r' data-(?:site-url|route)="[^"]*"', '', template)
    catalog = json.loads((output / 'catalog.json').read_text())
    components = catalog['components']
    documents = json.loads((output / 'docs.json').read_text())
    if len(components) != 78:
        raise ValueError('Sharing catalog must retain all 78 components')
    extensions = catalog.get('extensions', [])
    if not isinstance(extensions, list) or any(not isinstance(c, dict) or not isinstance(c.get('name'), str) or not c['name'].strip() for c in extensions):
        raise ValueError('Invalid extension catalog')
    components = [*components, *extensions]
    slugs = [component_slug(c['name']) for c in components]
    if len(set(slugs)) != len(slugs) or any(not s for s in slugs):
        raise ValueError('Component slugs must be unique and nonempty')
    if len({d['slug'] for d in documents}) != len(documents):
        raise ValueError('Documentation slugs must be unique')
    image_bytes = (output / 'assets/cui-social.png').read_bytes()
    if image_bytes[:8] != b'\x89PNG\r\n\x1a\n' or struct.unpack('>II', image_bytes[16:24]) != (1200, 630):
        raise ValueError('Share image must be a 1200 by 630 PNG')
    image = urljoin(site_url, 'assets/cui-social.png')
    alt = 'CUI Explorer and C code composed from the real WebAssembly component preview.'
    navigation = '<nav aria-label="Static site index">' + ''.join(
        f'<a href="{escape(urljoin(site_url, "components/"+slug+"/"), quote=True)}">{escape(c["name"])}</a> '
        for c,slug in zip(components,slugs)) + ''.join(
        f'<a href="{escape(urljoin(site_url, "docs/"+d["slug"]+"/"), quote=True)}">{escape(d["title"])}</a> '
        for d in documents) + '</nav>'
    pages = []

    def page(path, title, description, route, fallback):
        canonical = urljoin(site_url, path)
        html = re.sub(r'<title>.*?</title>', lambda _match: '<!--cui-sharing:start-->'+metadata(title, description, canonical, image, alt)+'<!--cui-sharing:end-->', template, count=1, flags=re.S)
        if path:
            html = html.replace('<head>', '<head><base href="../../">', 1)
        html = html.replace('<body ', f'<body data-site-url="{escape(site_url, quote=True)}" data-route="{escape(route, quote=True)}" ', 1)
        html = html.replace('</body>', '<noscript><main>' + fallback + navigation + '</main></noscript></body>', 1)
        destination = output / path / 'index.html'
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(html, encoding='utf-8')
        pages.append({'path': path, 'url': canonical, 'title': title, 'route': route})

    description = 'Build portable C11 interfaces with CUI: themed components, explicit state, composable behavior, live WebAssembly examples and browsable documentation.'
    page('', 'CUI · Portable C11 UI', description, 'home', '<h1>CUI: portable C11 interfaces</h1><p>'+escape(description)+'</p>')
    for component, slug in zip(components, slugs):
        name = component['name']
        description = f'Explore CUI {name}: its C11 API, interactive WebAssembly example, application-owned state and implementation scope. Complete component coverage remains in progress.'
        fallback = '<h1>'+escape(name)+'</h1><p>'+escape(description)+'</p><ul>' + ''.join('<li>'+escape(limit)+'</li>' for limit in component.get('limitations', [])) + '</ul>'
        page('components/'+slug+'/', name+' · CUI', description, name, fallback)
    page('examples/northstar/', 'Northstar workspace · CUI',
         'Try a native CUI workspace composition: navigation, account menu, pricing actions and editable settings.',
         'examples/northstar', '<h1>Northstar workspace</h1><p>A real CUI composition with session-local example actions.</p><p><a href="../../docs/composed-apps/">Read the buildable example guide</a></p>')
    for document in documents:
        if not re.fullmatch(r'[\w-]+', document['slug']):
            raise ValueError('Unsafe documentation slug')
        paragraphs = re.findall(r'<p>(.*?)</p>', document['html'], flags=re.S)
        description = plain_text(paragraphs[0] if paragraphs else document['title'])[:180]
        fallback = re.sub(r'href="#docs/([^"/]+)(?:/([^"]+))?"',
                          lambda match: 'href="'+escape(urljoin(site_url, 'docs/'+match[1]+'/')+
                          ('#doc-'+unquote(match[2]) if match[2] else ''), quote=True)+'"',
                          document['html'])
        page('docs/'+document['slug']+'/', document['title']+' · CUI', description, 'docs/'+document['slug'], '<h1>'+escape(document['title'])+'</h1>'+fallback)
        if document.get('markdown'):
            destination = output / ('docs/' + document['slug'] + '/index.html')
            markup = destination.read_text()
            alternate = '<link rel="alternate" type="text/markdown" href="' + escape(urljoin(site_url, document['markdown']), quote=True) + '">'
            destination.write_text(markup.replace('</head>', alternate + '</head>', 1))
    (output / 'routes.json').write_text(json.dumps(pages, ensure_ascii=False, indent=2)+'\n')
    root = ET.Element('urlset', xmlns='http://www.sitemaps.org/schemas/sitemap/0.9')
    for entry in pages:
        ET.SubElement(ET.SubElement(root, 'url'), 'loc').text = entry['url']
    ET.ElementTree(root).write(output / 'sitemap.xml', encoding='utf-8', xml_declaration=True)
    print(f'Prerendered metadata and no-JavaScript content for {len(pages)} shareable pages.')
    return pages


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--site-url', default=DEFAULT_SITE)
    args = parser.parse_args()
    build(args.output, args.site_url)
