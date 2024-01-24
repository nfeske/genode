MIRROR_FROM_REP_DIR := lib/mk/vfs_bdf.mk lib/mk/bdf_font.mk \
                       src/lib/vfs/ttf/glyphs_file_system.h src/lib/bdf_font \
                       src/lib/vfs/bdf

content: $(MIRROR_FROM_REP_DIR) LICENSE

$(MIRROR_FROM_REP_DIR):
	$(mirror_from_rep_dir)

LICENSE:
	cp $(GENODE_DIR)/LICENSE $@
