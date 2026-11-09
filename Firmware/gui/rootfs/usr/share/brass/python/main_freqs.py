import trx
import lv
from lv_const import *

from style import *
from layout import *

class MainFreqs(lv.obj):
	def __init__(self, parent = None):
		lv.obj.__init__(self, parent)

		self.clear_flag(LV_OBJ_FLAG_SCROLLABLE)
		self.add_style(main_freqs_style, LV_PART_MAIN)

		self.rx = None
		self.tx = None
		self.fft = None
		self.zoom = None

		self.makeFreq()
		self.makeRange()

	def makeFreq(self):
		freq = lv.label(self)
		freq.add_style(main_freqs_center_style, LV_PART_MAIN)

		freq.msg_subscribe(trx.MSG_FREQ_RX_CHANGED, self.msg_freq_rx)
		freq.msg_subscribe(trx.MSG_FREQ_TX_CHANGED, self.msg_freq_tx)
		self.freq = freq

	def makeRange(self):
		left = lv.label(self)
		left.add_style(main_freqs_left_style, LV_PART_MAIN)
		self.left = left

		right = lv.label(self)
		right.add_style(main_freqs_right_style, LV_PART_MAIN)
		self.right = right

		left.msg_subscribe(trx.MSG_FREQ_FFT_CHANGED, self.msg_fft_changed)
		left.msg_subscribe(trx.MSG_RATE_FFT_CHANGED, self.msg_rate_fft)

	def view_freq(self):
		if self.rx == self.tx:
			self.freq.set_text(f"{self.rx:,}")
		else:
			self.freq.set_text(f"{self.rx:,}" + "/" + f"{self.tx:,}")

	def view_range(self):
		left = int((self.fft - int(50000 / self.zoom)) / 1000)
		right = int((self.fft + int(50000 / self.zoom)) / 1000)

		self.left.set_text(f"{left:,}")
		self.right.set_text(f"{right:,}")

	def msg_freq_rx(self, msg, payload = None):
		self.rx = payload

		if self.tx:
			self.view_freq()

	def msg_freq_tx(self, msg, payload = None):
		self.tx = payload

		if self.rx:
			self.view_freq()

	def msg_fft_changed(self, msg, payload = None):
		self.fft = payload

		if self.zoom:
			self.view_range()

	def msg_rate_fft(self, msg, payload = None):
		self.zoom = payload

		if self.fft:
			self.view_range()
