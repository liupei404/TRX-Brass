import trx
import lv
from lv_const import *

from style import *

class Buttons(lv.obj):
	def __init__(self, parent = None):
		lv.obj.__init__(self, parent)

		self.clear_flag(LV_OBJ_FLAG_SCROLLABLE)
		self.add_style(buttons_style, LV_PART_MAIN)
		self.btn = []

		for i in range(6):
			btn = lv.btn(self)
			self.btn.append(btn)

			btn.remove_style_all()
			btn.add_style(button_style, LV_PART_MAIN)
			btn.set_pos(i * 133, 20)

			trx.connect_button(i, btn)
