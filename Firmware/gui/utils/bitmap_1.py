import cairo

def bitmap(width, height, name, shadow = 5.0):
	corner = 8
	lines = 2

	chip_big = 0
	chip_small = 0

	#chip_big = 21
	#chip_small = 7
	#chip_height = 4

	surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
	ctx = cairo.Context(surface)

	border = lines * 0.5 + 0.5
	width -= border * 2 + shadow
	height -= border * 2 + shadow

	ctx.set_line_join(cairo.LINE_JOIN_ROUND)
	ctx.set_line_cap(cairo.LINE_CAP_ROUND)

	ctx.set_line_width(0)

	# Shadow

	if shadow > 0:
		ctx.set_source_rgba(0, 0, 0, 0.5)

		ctx.move_to(border + corner + shadow, border + shadow)
		ctx.rel_line_to(width - corner, 0)
		ctx.rel_line_to(0, height - corner)
		ctx.rel_line_to(-corner, corner)
		ctx.rel_line_to(-width + corner, 0)
		ctx.rel_line_to(0, -height + corner)
		ctx.rel_line_to(corner, -corner)
		ctx.fill()

	# Background

	ctx.set_source_rgba(0x00 / 255.0, 0x38 / 255.0, 0x57 / 255.0, 0.75)

	ctx.move_to(border + corner, border)
	ctx.rel_line_to(width - corner, 0)
	ctx.rel_line_to(0, height - corner)
	ctx.rel_line_to(-corner, corner)
	ctx.rel_line_to(-width + corner, 0)
	ctx.rel_line_to(0, -height + corner)
	ctx.rel_line_to(corner, -corner)
	ctx.fill()

	# Border

	ctx.set_line_width(lines)
	ctx.set_source_rgba(0xFF / 255.0, 0xBB / 255.0, 0x01 / 255.0, 1.0)

	ctx.move_to(border, border + corner)
	ctx.rel_line_to(corner, -corner)
	ctx.rel_line_to(width - corner, 0)
	ctx.rel_line_to(0, corner)
	ctx.rel_move_to(0, height - 2 * corner)
	ctx.rel_line_to(-corner, corner)
	ctx.rel_line_to(-width + corner, 0)
	ctx.rel_line_to(0, -corner)
	ctx.stroke()

	# Chips top

	if chip_big > 0:
		ctx.move_to(border + corner + 10.0, border + chip_height * 0.5)
		ctx.set_line_cap(cairo.LINE_CAP_ROUND)
		ctx.set_line_width(chip_height)
		ctx.rel_line_to(chip_big, 0)
		ctx.stroke()

		ctx.move_to(border + corner + 10.0 - chip_height * 0.25, border + chip_height * 0.25)
		ctx.set_line_cap(cairo.LINE_CAP_SQUARE)
		ctx.set_line_width(chip_height * 0.5)
		ctx.rel_line_to(chip_big + chip_height * 0.5, 0)
		ctx.stroke()

	if chip_small > 0:
		ctx.move_to(border + width - 20.0 - chip_small, border + chip_height * 0.5)
		ctx.set_line_cap(cairo.LINE_CAP_ROUND)
		ctx.set_line_width(chip_height)
		ctx.rel_line_to(chip_small, 0)
		ctx.stroke()

		ctx.move_to(border + width - 20.0 - chip_small - chip_height *0.25, border + chip_height * 0.25)
		ctx.set_line_cap(cairo.LINE_CAP_ROUND)
		ctx.set_line_width(chip_height * 0.5)
		ctx.rel_line_to(chip_small + chip_height * 0.5, 0)
		ctx.stroke()

	# Chips bottom

	if chip_small > 0:
		ctx.move_to(border + corner + 10.0, border + height - chip_height * 0.5)
		ctx.set_line_cap(cairo.LINE_CAP_ROUND)
		ctx.set_line_width(chip_height)
		ctx.rel_line_to(chip_small, 0)
		ctx.stroke()

		ctx.move_to(border + corner + 10.0 - chip_height * 0.25, border + height - chip_height * 0.25)
		ctx.set_line_cap(cairo.LINE_CAP_ROUND)
		ctx.set_line_width(chip_height * 0.5)
		ctx.rel_line_to(chip_small + chip_height * 0.5, 0)
		ctx.stroke()

	if chip_big > 0:
		ctx.move_to(border + width - 10.0 - chip_big - corner, border + height - chip_height * 0.5)
		ctx.set_line_cap(cairo.LINE_CAP_ROUND)
		ctx.set_line_width(chip_height)
		ctx.rel_line_to(chip_big, 0)
		ctx.stroke()

		ctx.move_to(border + width - 10.0 - chip_big - corner - chip_height * 0.25, border + height - chip_height * 0.25)
		ctx.set_line_cap(cairo.LINE_CAP_SQUARE)
		ctx.set_line_width(chip_height * 0.5)
		ctx.rel_line_to(chip_big + chip_height * 0.5, 0)
		ctx.stroke()

	surface.write_to_png(name)
