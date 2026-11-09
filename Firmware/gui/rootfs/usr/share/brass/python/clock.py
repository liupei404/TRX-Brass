import trx
import lv
from lv_const import *
from datetime import datetime

from style import *
from layout import *

class Clock(lv.obj):
	def __init__(self, parent = None):
		lv.obj.__init__(self, parent)

		self.clear_flag(LV_OBJ_FLAG_SCROLLABLE)
		self.add_style(clock_style, LV_PART_MAIN)

		clock_label = lv.label(self)
		clock_label.add_style(clock_label_style, LV_PART_MAIN)
		self.clock_label = clock_label

		bat = lv.label(self)
		bat.add_style(bat_style, LV_PART_MAIN)
		bat.set_text(LV_SYMBOL_BATTERY_FULL)
		self.bat = bat

		self.timer_create(self.update, 1000)
		self.update()

	def update(self):
		now = datetime.now()
		self.clock_label.set_text(now.strftime("%H:%M:%S"))
