import trx
import lv

from lv_const import *

from top import *
from main_spectrum import *
from main_spectrum3d import *
from main_freqs import *
from main_waterfall import *
from buttons import *
from msg import *
from msg_tiny import *

class MainScreen(lv.obj):
	def __init__(self, parent = None):

		lv.obj.__init__(self, parent)

		self.clear_flag(LV_OBJ_FLAG_SCROLLABLE)

		self.spectrum = MainSpectrum(self)
		trx.connect_spectrum(self.spectrum)

		if True:
			self.waterfall = MainWaterfall(self)
			trx.connect_waterfall(self.waterfall)
		else:
			self.spectrum3d = MainSpectrum3D(self)
			trx.connect_spectrum3d(self.spectrum3d)

		self.top = Top(self)
		self.freqs = MainFreqs(self)
		self.buttons = Buttons(self)

		self.msg = Msg(self)
		self.msg_tiny = MsgTiny(self)

def main():
	return MainScreen()
