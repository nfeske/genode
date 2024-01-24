BDF_FONTS = $(notdir $(wildcard $(REP_DIR)/recipes/raw/atari_fonts_fs/*.bdf))

content: fonts_fs.config $(BDF_FONTS)

fonts_fs.config $(BDF_FONTS):
	cp $(REP_DIR)/recipes/raw/atari_fonts_fs/$@ $@
