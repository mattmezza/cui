#!/usr/bin/env python3
"""Check published docs, API headers and native downloads against one source revision."""
import argparse
import hashlib
import io
import json
import tarfile
import time
from urllib.parse import urlencode, urljoin
from urllib.request import urlopen


def verify(base, revision):
    def fetch(path):
        url = urljoin(base, path) + '?' + urlencode({'revision': revision})
        with urlopen(url, timeout=20) as response:
            return response.read()

    info = json.loads(fetch('build-info.json'))
    if info['source_revision'] != revision:
        raise ValueError('Published revision has not reached this endpoint yet')
    catalog = json.loads(fetch('catalog.json'))
    if len(catalog['components']) != 78:
        raise ValueError('Published component catalog lost entries')
    api = json.loads(fetch('api-index.json'))
    names = {symbol['name'] for symbol in api['symbols']}
    if api['source_revision'] != revision or not {'cui_first_diagnostic', 'cui_text_ellipsis', 'cui_icon_query'} <= names:
        raise ValueError('Published API index does not match the developer APIs')
    archive = fetch('downloads/cui-native.tar.gz')
    digest = hashlib.sha256(archive).hexdigest()
    if digest != info['downloads']['downloads/cui-native.tar.gz']['sha256']:
        raise ValueError('Native archive differs from build metadata')
    if fetch('downloads/cui-native.tar.gz.sha256').decode().split()[0] != digest:
        raise ValueError('Native archive checksum file differs')
    with tarfile.open(fileobj=io.BytesIO(archive), mode='r:gz') as bundle:
        for name in ('cui.h', 'cui_icon.h', 'cui_text_overflow.h'):
            header = fetch('headers/' + name)
            expected = info['headers_sha256']['include/cui/' + name]
            if hashlib.sha256(header).hexdigest() != expected:
                raise ValueError('Published public header differs: ' + name)
            member = bundle.extractfile('cui-native/vendor/include/cui/' + name)
            if member is None or member.read() != header:
                raise ValueError('Native archive and public header differ: ' + name)
    for slug in ('layout_guide', 'debugging', 'native-app-starter'):
        page = fetch('docs/' + slug + '/').decode()
        if 'rel="alternate" type="text/markdown"' not in page:
            raise ValueError('Missing Markdown discovery on ' + slug)
    for task in ('native-app', 'layout', 'debugging'):
        if not fetch('tasks/' + task + '.md').startswith(b'# CUI'):
            raise ValueError('Missing focused reading set: ' + task)
    print(f'Verified published developer docs, 78 entries, API headers and native archive at {revision}.', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--url', default='https://matteo.merola.co/cui/')
    parser.add_argument('--revision', required=True)
    parser.add_argument('--attempts', type=int, default=6)
    args = parser.parse_args()
    for attempt in range(args.attempts):
        try:
            verify(args.url.rstrip('/') + '/', args.revision)
            return
        except (OSError, ValueError, KeyError, tarfile.TarError) as error:
            if attempt + 1 == args.attempts:
                raise SystemExit(str(error)) from error
            print(f'Publication check {attempt + 1}: {error}; retrying.', flush=True)
            time.sleep(5)


if __name__ == '__main__':
    main()
