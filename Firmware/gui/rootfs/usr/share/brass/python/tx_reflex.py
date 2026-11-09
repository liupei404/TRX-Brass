import trx
import lv
from lv_const import *

from style import *
from layout import *

class TXReflex(lv.finder):
	def __init__(self, parent = None):
		lv.finder.__init__(self, parent)

		self.add_style(finder_style, LV_PART_MAIN)
		self.add_style(tx_reflex_indicator_style, LV_PART_INDICATOR)
		self.add_style(tx_reflex_style, LV_PART_MAIN)
