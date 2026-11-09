import trx
import lv

from lv_const import *

from style import *
from layout import *

class MainSpectrum3D(lv.spectrum3d):
	def __init__(self, parent = None):
		lv.spectrum3d.__init__(self, parent)

		self.clear_flag(LV_OBJ_FLAG_SCROLLABLE)

		self.add_style(waterfall_style, LV_PART_MAIN)

		self.set_data_size(2048, 100)
		self.clear_data()
		self.set_grad(waterfall_grad)
