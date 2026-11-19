#!/usr/bin/env python3
"""Writes Data/plugin-knowledge.json, the plug-in descriptions Torqwave ships for Ask Torq, from a
plug-in catalogue (the app's Plug-in Catalogue.json) and a review of its descriptions.

The review is a JSON object, one entry for each description by the catalogue's key (maker|name,
lower case):
    {"<maker|name>": {"action": "keep"}}                 shipped as the model wrote it
    {"<maker|name>": {"action": "fix", "text": "..."}}   shipped as the reviewer rewrote it
    {"<maker|name>": {"action": "drop", "why": "..."}}   not shipped
Only reviewed descriptions are shipped: one the review doesn't name is left out.

What is shipped for each plug-in: its maker, name and category and the description, who wrote
it and when it was reviewed. Nothing else from the catalogue: no paths, versions or parameters.
The text is refused if it holds anything that looks like a path, an address, an email or a key.

Usage: export_plugin_knowledge.py <catalogue.json> <review.json> [--reviewer NAME] [--out FILE]
Standard library only.
"""
import argparse
import datetime
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
PRIVATE = [
    (re.compile(r'/(Users|home)/'), 'a home path'),
    (re.compile(r'[A-Za-z]:\\'), 'a Windows path'),
    (re.compile(r'\b\d{1,3}(\.\d{1,3}){3}\b'), 'an IP address'),
    (re.compile(r'[\w.+-]+@[\w-]+\.[\w.-]+'), 'an email address'),
    (re.compile(r'\b(sk|pk|api|key|token|bearer)[-_ ]?[A-Za-z0-9]{16,}', re.I), 'a key'),
    (re.compile(r'https?://', re.I), 'a URL'),
]


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('catalogue')
    parser.add_argument('review')
    parser.add_argument('--reviewer', default='reviewed')
    parser.add_argument('--out', default=str(ROOT / 'Data' / 'plugin-knowledge.json'))
    args = parser.parse_args()

    catalogue = json.loads(pathlib.Path(args.catalogue).read_text(encoding='utf-8'))
    review = json.loads(pathlib.Path(args.review).read_text(encoding='utf-8'))
    understanding = catalogue.get('understanding', {})

    # Name, maker and category from the facts, by the same key the understanding uses.
    about = {}
    for entry in catalogue.get('facts', {}).values():
        key = (entry.get('maker', '').strip() + '|' + entry.get('name', '').strip()).lower()
        if key not in about or (not about[key]['category'] and entry.get('category')):
            about[key] = {'name': entry.get('name', ''), 'maker': entry.get('maker', ''), 'category': entry.get('category', '')}

    reviewed = datetime.datetime.now(datetime.timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')
    plugins, problems = {}, []
    kept = fixed = dropped = 0

    for key, decision in sorted(review.items()):
        action = decision.get('action')
        written = understanding.get(key)

        if action == 'drop':
            dropped += 1
            continue
        if action not in ('keep', 'fix'):
            problems.append(f'{key}: unknown action {action!r}')
            continue
        if written is None and action == 'keep':
            problems.append(f'{key}: kept, but the catalogue has no description of it')
            continue

        text = (decision.get('text') if action == 'fix' else written['text']) or ''
        text = text.strip()
        if not text:
            problems.append(f'{key}: empty text')
            continue

        for pattern, what in PRIVATE:
            if pattern.search(text):
                problems.append(f'{key}: the text holds what looks like {what}')

        names = about.get(key) or {'name': key.split('|', 1)[-1], 'maker': key.split('|', 1)[0], 'category': ''}
        model = (written or {}).get('model', '')
        plugins[key] = {
            'name': names['name'],
            'maker': names['maker'],
            'category': names['category'],
            'about': text,
            'written': (model + ', revised by ' + args.reviewer) if action == 'fix' and model else (model or args.reviewer),
            'reviewed': reviewed,
        }
        kept += action == 'keep'
        fixed += action == 'fix'

    unreviewed = sorted(set(understanding) - set(review))

    if problems:
        print('\n'.join(problems), file=sys.stderr)
        return 1

    out = pathlib.Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps({'version': 1, 'plugins': plugins}, indent=1, ensure_ascii=False, sort_keys=True) + '\n', encoding='utf-8')
    print(f'{out}: {len(plugins)} plug-ins ({kept} kept, {fixed} rewritten); {dropped} dropped; {len(unreviewed)} unreviewed left out')
    return 0


if __name__ == '__main__':
    sys.exit(main())
