import trx
import lv
from lv_const import *

from style import *

class SMeter(lv.xmeter):
	def __init__(self, parent = None):
		lv.xmeter.__init__(self, parent)

		self.add_style(smeter_style, LV_PART_MAIN)
		self.add_style(smeter_slice_style, LV_PART_INDICATOR)
		self.add_style(smeter_part1_style, LV_PART_CUSTOM_FIRST)
		self.add_style(smeter_part2_style, LV_PART_CUSTOM_FIRST + 0x10000)
		self.add_style(smeter_part3_style, LV_PART_CUSTOM_FIRST + 0x20000)
		self.add_style(smeter_part4_style, LV_PART_CUSTOM_FIRST + 0x30000)

		self.set_range(-121, -73 + 40)
		self.set_slice(3)

		self.set_label(0, "S1", -121)
		self.set_label(1, "3", -109)
		self.set_label(2, "5", -97)
		self.set_label(3, "7", -85)
		self.set_label(4, "9", -73)
		self.set_label(5, "+20", -73 + 20)
		self.set_label(6, "+40", -73 + 40)

		self.set_part(0, -134)
		self.set_part(1, -73)
		self.set_part(2, -73 + 20)

		trx.connect_smeter(self)
