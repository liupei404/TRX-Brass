import trx
import lv
from lv_const import *

from rx_finder import *
from tx_finder import *
from bandinfo import *

from style import *
from layout import *

class MainSpectrum(lv.spectrum):
	def __init__(self, parent = None):
		lv.spectrum.__init__(self, parent)

		self.clear_flag(LV_OBJ_FLAG_SCROLLABLE)

		self.add_style(spectrum_style, LV_PART_MAIN)
		self.add_style(spectrum_lines_style, LV_PART_INDICATOR)
		self.add_style(spectrum_peaks_style, LV_PART_TICKS)

#		self.set_peak(True)
#		self.set_peak_hold(3000)
#		self.set_peak_speed(1)

		self.set_data_size(400)
		self.clear_data()

		self.tx_finder = TXFinder(self)
		self.tx_finder.add_style(spectrum_tx_finder_style, LV_PART_MAIN)

		trx.connect_tx_finder(self.tx_finder)

		self.rx_finder = RXFinder(self)
		self.rx_finder.add_style(spectrum_rx_finder_style, LV_PART_MAIN)

		trx.connect_rx_finder(self.rx_finder)
