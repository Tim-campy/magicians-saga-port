"""Synthetic structural OPT fixtures, no original assets."""
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

spec=importlib.util.spec_from_file_location('inspect_opt',Path(__file__).resolve().parents[1]/'tools/inspect_opt.py')
opt=importlib.util.module_from_spec(spec)
sys.modules[spec.name]=opt
spec.loader.exec_module(opt)
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import render_opt_reference as reference
from PIL import Image


class OptTests(unittest.TestCase):
    def test_grid_order_signed_shorts_and_unsigned_extent(self):
        raw=bytes([128,200,2,1,1])+struct.pack('>7h',-1,-32768,32767,3,4,5,6)+bytes([0])
        obj=opt.parse(raw)
        self.assertEqual((obj.cell_width,obj.cell_height),(128,200))
        self.assertEqual(obj.cells,(((-1,-32768,32767,3,4,5,6),),()))
        self.assertEqual(opt.encode(obj),raw)

    def test_multirecord_cell_and_empty_grid(self):
        raw=bytes([40,80,1,1,2])+struct.pack('>14h',*range(14))
        self.assertEqual(len(opt.parse(raw).cells[0]),2)
        self.assertEqual(opt.encode(opt.parse(raw)),raw)
        self.assertEqual(opt.encode(opt.parse(bytes([0,0,0,0]))),bytes(4))

    def test_all_truncations_and_trailing_data(self):
        raw=bytes([30,30,1,1,1])+bytes(14)
        for end in range(len(raw)):
            with self.subTest(end=end),self.assertRaises(ValueError):opt.parse(raw[:end])
        with self.assertRaises(ValueError):opt.parse(raw+b'\0')

    def test_rejects_unknown_variant_negative_counts(self):
        for raw in (bytes([255,1,1,1]),bytes([1,1,128,1]),bytes([1,1,1,255]),bytes([1,1,1,1,128])):
            with self.subTest(raw=raw.hex()),self.assertRaises(ValueError):opt.parse(raw)

    def test_encoder_grid_and_field_ranges(self):
        for obj in (opt.Opt(1,1,2,1,((),)),opt.Opt(255,1,0,0,()),
                    opt.Opt(1,1,1,1,(((1,2),),)),
                    opt.Opt(1,1,1,1,(((32768,0,0,0,0,0,0),),))):
            with self.subTest(obj=obj),self.assertRaises(ValueError):opt.encode(obj)

    def test_reference_preserves_rgba_and_negative_origin(self):
        source=Image.new('RGBA',(2,1))
        source.putdata([(10,20,30,0),(90,80,70,127)])
        layout=opt.Opt(10,10,2,1,(((-1,-2,1,0,0,1,1),),((-1,0,1,1,0,1,1),)))
        canvas,report=reference.render_reference(layout,source)
        self.assertEqual(report['occupied_bounds'],[-2,1,11,2])
        self.assertEqual(canvas.getpixel((0,0)),(10,20,30,0))
        self.assertEqual(canvas.getpixel((12,0)),(90,80,70,127))
        self.assertEqual(canvas.getpixel((1,0)),(0,0,0,0))
        self.assertEqual(report['assigned_pixels'],2)
        self.assertEqual(report['unassigned_padding_pixels'],11)
        self.assertEqual(report['transparent_assigned_pixels'],1)
        with self.assertRaises(ValueError):reference.render_reference(layout,source,True)

    def test_reference_rejects_ambiguous_composition_and_bad_crops(self):
        source=Image.new('RGBA',(2,2))
        for cell in (((-1,0,0,0,0,2,2),(-1,1,1,0,0,1,1)),
                     ((-1,0,0,1,1,2,2),), ((0,0,0,0,0,1,1),)):
            with self.subTest(cell=cell),self.assertRaises(ValueError):
                reference.render_reference(opt.Opt(2,2,1,1,(cell,)),source)

    def test_reference_row_major_placement(self):
        source=Image.new('RGBA',(2,1));source.putdata([(1,2,3,255),(4,5,6,255)])
        layout=opt.Opt(3,4,1,2,(((-1,0,0,0,0,1,1),),((-1,0,0,1,0,1,1),)))
        canvas,report=reference.render_reference(layout,source,True)
        self.assertEqual(canvas.getpixel((0,0)),(1,2,3,255))
        self.assertEqual(canvas.getpixel((0,4)),(4,5,6,255))
        self.assertEqual(report['canvas_size'],[1,5])


if __name__=='__main__':unittest.main()
