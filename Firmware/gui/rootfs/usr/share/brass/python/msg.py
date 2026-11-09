import trx
import lv
from lv_const import *

from style import *

class Msg(lv.hiding):
	def __init__(self, parent = None):
		lv.hiding.__init__(self, parent)

		self.clear_flag(LV_OBJ_FLAG_SCROLLABLE)
		self.add_style(msg_style, LV_PART_MAIN)

		self.set_timeout(2000)

		label = lv.label(self)

		label.add_style(msg_label_normal_style, LV_PART_MAIN)
		label.set_text("")

		self.label = label
		self.msg_subscribe(trx.MSG_MSG, self.msg_msg)
		self.msg_subscribe(trx.MSG_MSG_LONG, self.msg_msg)

	def msg_msg(self, msg, payload = None):
		if msg == trx.MSG_MSG_LONG:
			self.set_timeout(60000)
		else:
			self.set_timeout(2000)

		if payload[0] == "^":
			payload = payload[1:]
			self.label.add_style(msg_label_normal_style, LV_PART_MAIN)
		elif payload[0] == "#":
			payload = payload[1:]
			self.label.add_style(msg_label_select_style, LV_PART_MAIN)
		else:
			self.label.add_style(msg_label_normal_style, LV_PART_MAIN)

		self.move_foreground()
		self.label.set_text(payload)
		self.touch()
