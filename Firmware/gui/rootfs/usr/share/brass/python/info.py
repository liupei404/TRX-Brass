import trx
import lv
from lv_const import *

from style import *
from layout import *

class Info(lv.obj):
	def __init__(self, parent = None):
		lv.obj.__init__(self, parent)

		self.clear_flag(LV_OBJ_FLAG_SCROLLABLE)
		self.add_style(info_style, LV_PART_MAIN)

		self.modes = {
			trx.RADIO_MODE_LSB: "LSB",
			trx.RADIO_MODE_USB: "USB",
			trx.RADIO_MODE_CW: "CW",
			trx.RADIO_MODE_CWR: "CW-R",
			trx.RADIO_MODE_AM: "AM",
			trx.RADIO_MODE_NFM: "NFM",
			trx.RADIO_MODE_RTTY: "RTTY",
			trx.RADIO_MODE_OLIVIA: "OLIV",
			trx.RADIO_MODE_DMR: "DMR"
		}

		self.split = {
			trx.SPLIT_NONE: "RX/TX",
			trx.SPLIT_RX: "RX",
			trx.SPLIT_TX: "TX"
		}

		self.agc = {
			trx.AGC_OFF: "OFF",
			trx.AGC_LONG: "LONG",
			trx.AGC_SLOW: "SLOW",
			trx.AGC_MED: "MED",
			trx.AGC_FAST: "FAST",
			trx.AGC_CUSTOM: "CUST"
		}

		split_label = lv.label(self)
		split_label.add_style(info_split_style, LV_PART_MAIN)
		self.split_label = split_label

		mode_label = lv.label(self)
		mode_label.add_style(info_mode_style, LV_PART_MAIN)
		self.mode_label = mode_label

		agc_label = lv.label(self)
		agc_label.add_style(info_agc_style, LV_PART_MAIN)
		self.agc_label = agc_label

		pre_label = lv.label(self)
		pre_label.set_text("PRE")
		pre_label.add_style(info_pre_style, LV_PART_MAIN)
		pre_label.add_style(info_off_style, LV_PART_MAIN)
		self.pre_label = pre_label

		att_label = lv.label(self)
		att_label.set_text("ATT")
		att_label.add_style(info_att_style, LV_PART_MAIN)
		att_label.add_style(info_off_style, LV_PART_MAIN)
		self.att_label = att_label

		atu_label = lv.label(self)
		atu_label.add_style(info_atu_style, LV_PART_MAIN)
		atu_label.add_style(info_off_style, LV_PART_MAIN)
		self.atu_label = atu_label

		self.msg_subscribe(trx.MSG_MODE_CHANGED, self.msg_mode_changed)
		self.msg_subscribe(trx.MSG_AGC_CHANGED, self.msg_agc_changed)
		self.msg_subscribe(trx.MSG_SPLIT_CHANGED, self.msg_split_changed)
		self.msg_subscribe(trx.MSG_ANT_CHANGED, self.msg_ant_changed)

	def msg_mode_changed(self, msg, payload = None):
		self.mode_label.set_text(self.modes[payload])

	def msg_agc_changed(self, msg, payload = None):
		self.agc_label.set_text(self.agc[payload])

	def msg_split_changed(self, msg, payload = None):
		self.split_label.set_text(self.split[payload])

	def msg_ant_changed(self, msg, payload = None):
		self.atu_label.set_text(f"ATU{payload}")
