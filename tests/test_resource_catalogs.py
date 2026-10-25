import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
from audit_resource_catalogs import audit, parse_catalog


class CatalogTests(unittest.TestCase):
    def test_preserves_duplicates_options_and_unindexed_rows(self):
        rows = parse_catalog(b'\xef\xbb\xbf 2\ta.gif,bin,q+1\r\n2\tb.png\r\nc.seb\r\n')
        self.assertEqual([x['explicit_id'] for x in rows], [2, 2, None])
        self.assertEqual(rows[0]['options'], ['bin', 'q+1'])
        self.assertEqual(rows[2]['line'], 3)

    def test_rejects_unrecognized_columns(self):
        with self.assertRaises(ValueError):
            parse_catalog(b'1\tx\ty')

    def test_candidate_scope_and_integrity(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            members = []
            for i, (container, name, data) in enumerate([
                ('a', 'en/img.inf', b'0\told.gif'),
                ('a', 'old.png', b'root'), ('a', 'en/old.png', b'locale'),
                ('a', 'seb.inf', b'0\tabsent.seb'),
                ('a', 'absent.png', b'not-a-seb'),
                ('b', 'absent.seb', b'other-container')]):
                output = str(i)+'.bin'
                (root/output).write_bytes(data)
                members.append(dict(container=container, name=name, output=output,
                                    size=len(data), sha256=hashlib.sha256(data).hexdigest()))
            manifest = root/'manifest.json'
            manifest.write_text(json.dumps(dict(apk_sha256='fixture', members=members)))
            result = audit(manifest)
            image, seb = result['catalogs']
            self.assertEqual({x['name'] for x in image['rows'][0]['candidates']},
                             {'old.png', 'en/old.png'})
            self.assertEqual(seb['rows'][0]['candidates'], [])
            (root/'1.bin').write_bytes(b'ROOT')
            with self.assertRaisesRegex(ValueError, 'identity mismatch'):
                audit(manifest)


if __name__ == '__main__':
    unittest.main()
