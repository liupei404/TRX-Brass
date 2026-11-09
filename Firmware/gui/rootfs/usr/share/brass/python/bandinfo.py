import trx
import lv
from lv_const import *

from style import *
from layout import *

class BandInfo(lv.bandinfo):
	def __init__(self, parent = None):
		lv.bandinfo.__init__(self, parent)

		self.clear_flag(LV_OBJ_FLAG_SCROLLABLE)
		self.set_timeout(1000)

		self.add_style(bandinfo_style, LV_PART_MAIN)
		self.add_style(bandinfo_indicator_style, LV_PART_INDICATOR)
		self.add_style(bandinfo_marker_style, LV_PART_TICKS)

		self.msg_subscribe(trx.MSG_FREQ_FFT_CHANGED, self.msg_fft_changed)
		self.msg_subscribe(trx.MSG_RATE_FFT_CHANGED, self.msg_rate_fft)
		self.msg_subscribe(trx.MSG_FREQ_RX_CHANGED, self.msg_freq)
		self.msg_subscribe(trx.MSG_FREQ_TX_CHANGED, self.msg_freq)

	def msg_fft_changed(self, msg, payload = None):
		self.set_center(payload)

	def msg_rate_fft(self, msg, payload = None):
		self.set_span(int(100000 / payload))

	def msg_freq(self, msg, payload = None):
		self.touch()
