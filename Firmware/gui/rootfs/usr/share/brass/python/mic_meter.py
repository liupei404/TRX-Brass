import trx
import lv
from lv_const import *

from style import *

class MicMeter(lv.xmeter):
	def __init__(self, parent = None):
		lv.xmeter.__init__(self, parent)

		self.add_style(smeter_style, LV_PART_MAIN)
		self.add_style(smeter_slice_style, LV_PART_INDICATOR)
		self.add_style(smeter_part1_style, LV_PART_CUSTOM_FIRST)
		self.add_style(smeter_part2_style, LV_PART_CUSTOM_FIRST + 0x10000)
		self.add_style(smeter_part3_style, LV_PART_CUSTOM_FIRST + 0x20000)
		self.add_style(smeter_part4_style, LV_PART_CUSTOM_FIRST + 0x30000)

		self.set_range(0.0, 1.0)
		self.set_slice(0.035)

		self.set_label(0, "Mic", 0.0)
		self.set_label(1, "2", 0.2)
		self.set_label(2, "4", 0.4)
		self.set_label(3, "6", 0.6)
		self.set_label(4, "8", 0.8)
		self.set_label(5, "10", 1.0)

		self.set_part(0, 0.2)
		self.set_part(1, 0.5)
		self.set_part(2, 0.8)

		trx.connect_mic_meter(self)
