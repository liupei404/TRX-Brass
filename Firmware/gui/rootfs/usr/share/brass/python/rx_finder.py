import trx
import lv
from lv_const import *

from style import *
from layout import *

class RXFinder(lv.finder):
	def __init__(self, parent = None):
		lv.finder.__init__(self, parent)

		self.add_style(finder_style, LV_PART_MAIN)
		self.add_style(rx_finder_indicator_style, LV_PART_INDICATOR)
		self.add_style(rec_off_style, LV_PART_INDICATOR)

		self.msg_subscribe(trx.MSG_RECORDER, self.msg_recorder)

	def msg_recorder(self, msg, on = None):
		if on:
			self.add_style(rec_on_style, LV_PART_INDICATOR)
		else:
			self.add_style(rec_off_style, LV_PART_INDICATOR)
