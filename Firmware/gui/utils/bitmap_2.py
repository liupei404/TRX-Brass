import cairo

def bitmap(width, height, name, shadow = 5.0):
	corner = 10
	lines = 3
	embose = 3
	embose_lines = 1.5

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
		ctx.set_source_rgba(0, 0, 0, 0.60)

		ctx.move_to(border + corner + shadow, border + shadow)
		ctx.rel_line_to(width - corner, 0)
		ctx.rel_line_to(0, height - corner)
		ctx.rel_line_to(-corner, corner)
		ctx.rel_line_to(-width + corner, 0)
		ctx.rel_line_to(0, -height + corner)
		ctx.rel_line_to(corner, -corner)
		ctx.fill()

	# Background

	ctx.set_source_rgba(0x00 / 255.0, 0x25 / 255.0, 0x50 / 255.0, 0.9)

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
	ctx.set_source_rgba(1.0, 1.0, 1.0, 0.5)

	ctx.move_to(border, border + height - embose)
	ctx.rel_line_to(0, -height + corner + embose)
	ctx.rel_line_to(corner, -corner)
	ctx.rel_line_to(width - corner - embose, 0)
	ctx.stroke()

	if embose > 0:
		ctx.set_line_width(embose_lines)
		ctx.set_source_rgba(1.0, 1.0, 1.0, 0.25)

		ctx.move_to(border + width - embose, border + embose)
		ctx.rel_line_to(0, height - corner - embose - embose/2)
		ctx.rel_line_to(-corner + embose/2, corner - embose/2)
		ctx.rel_line_to(-width + corner + embose * 2, 0)
		ctx.stroke()

    #

	ctx.set_source_rgba(0, 0, 0, 0.5)

	ctx.move_to(border + width, border)
	ctx.rel_line_to(0, height - corner)
	ctx.rel_line_to(-corner, corner)
	ctx.rel_line_to(-width + corner, 0)
	ctx.stroke()

	if embose > 0:
		ctx.set_line_width(embose_lines)
		ctx.set_source_rgba(0, 0, 0, 0.5)

		ctx.move_to(border + embose, border + height - embose)
		ctx.rel_line_to(0, -height + corner + embose + embose/2)
		ctx.rel_line_to(corner - embose/2, -corner + embose/2)
		ctx.rel_line_to(width - corner - embose * 2, 0)
		ctx.stroke()

	surface.write_to_png(name)
