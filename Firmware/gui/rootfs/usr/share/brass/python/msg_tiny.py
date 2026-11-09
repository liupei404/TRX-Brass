import trx
import lv
from lv_const import *

from style import *
from layout import *

class MsgTiny(lv.hiding):
	def __init__(self, parent = None):
		lv.hiding.__init__(self, parent)

		self.clear_flag(LV_OBJ_FLAG_SCROLLABLE)
		self.set_timeout(1000)
		self.add_style(msg_tiny_style, LV_PART_MAIN)
		self.set_style_opa(0, LV_PART_MAIN)

		label = lv.label(self)
		label.add_style(msg_tiny_label_style, LV_PART_MAIN)
		label.set_text("")

		self.label = label
		self.msg_subscribe(trx.MSG_MSG_TINY, self.msg_msg_tiny)

	def msg_msg_tiny(self, msg, payload = None):
		self.move_foreground()
		self.label.set_text(payload)
		self.touch()
