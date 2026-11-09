import trx
import lv
from lv_const import *

from rx_finder import *
from bandinfo import *

class MainWaterfall(lv.waterfall):
	def __init__(self, parent = None):
		lv.waterfall.__init__(self, parent)

		self.clear_flag(LV_OBJ_FLAG_SCROLLABLE)

		self.add_style(waterfall_style, LV_PART_MAIN)

		self.set_data_size(2047)
		self.clear_data()
		self.set_grad(waterfall_grad)

		self.rx_finder = RXFinder(self)
		self.rx_finder.add_style(waterfall_rx_finder_style, LV_PART_MAIN)
		trx.connect_rx_finder(self.rx_finder)

		self.bandinfo = BandInfo(self)
		self.bandinfo.add_style(waterfall_bandinfo_style, LV_PART_MAIN)
