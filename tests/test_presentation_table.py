import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from inspect_presentation_table import parse_row, markup_inventory


class PresentationTableTests(unittest.TestCase):
    def test_markup_preserves_candidates_and_duplicate_ids(self):
        report = markup_inventory([
            {'id': 1, 'texts': ['<br><0><br>', '<unknown=x><incomplete']},
            {'id': 1, 'texts': ['<br>']}, {'id': 2, 'texts': None}], 'talk')
        self.assertEqual(report['text_count'], 3)
        self.assertEqual(report['tokens']['<br>']['count'], 3)
        self.assertEqual([x['record_ordinal'] for x in report['tokens']['<br>']['locations']], [0,0,1])
        self.assertTrue(report['tokens']['<0>']['numeric_argument_candidate'])
        self.assertFalse(report['tokens']['<unknown=x>']['numeric_argument_candidate'])
        self.assertNotIn('<incomplete', report['tokens'])

    def test_event_markup_does_not_treat_text_as_talk_array(self):
        report = markup_inventory([{'id': 3, 'text': '<po=cm><0>&<br>'}], 'event')
        self.assertEqual(report['text_count'], 1)
        self.assertEqual(set(report['tokens']), {'<po=cm>', '<0>', '<br>'})

    def test_talk_counts_empty_and_cr(self):
        record, rest = parse_row('7\t-1\t3\tfirst\t\tlast\r\textra', 'talk')
        self.assertEqual(record, {'id': 7, 'character_index': -1, 'texts': ['first', '', 'last\r']})
        self.assertEqual(rest, ['extra'])

    def test_null_and_empty_differ(self):
        self.assertIsNone(parse_row('1\t2\t-1\tunused', 'talk')[0]['texts'])
        self.assertEqual(parse_row('1\t2\t0', 'talk')[0]['texts'], [])
        self.assertEqual(parse_row('1\t2\t-2\tunused', 'talk')[1], ['unused'])

    def test_event_text_is_single_token(self):
        self.assertEqual(parse_row('3\t4\ta&b\r\textra', 'event'),
                         ({'id': 3, 'type': 4, 'text': 'a&b\r'}, ['extra']))

    def test_truncated_and_invalid_counts(self):
        for line in ['1\t2', '1\t2\t2\tonly one', '1\t2\t', '1\t2\t2147483648']:
            with self.subTest(line=line), self.assertRaises(ValueError):
                parse_row(line, 'talk')

    def test_signed_integer_subset(self):
        self.assertEqual(parse_row(' +7\t-2 \t1\t<0>', 'talk')[0]['id'], 7)
        for value in ['0x10', '1.0', 'NaN', '-2147483649']:
            with self.subTest(value=value), self.assertRaises(ValueError):
                parse_row(value + '\t0\ttext', 'event')


if __name__ == '__main__':
    unittest.main()
