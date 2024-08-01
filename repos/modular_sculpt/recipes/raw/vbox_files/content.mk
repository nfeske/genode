content: vbox_files.tar

TAR := tar --owner=0 --group=0 --numeric-owner --mode='go=' --mtime='1970-01-01 00:00+00'

FILES := ubuntu-22.04-raw \
		 windows-10-raw
vbox_files.tar:
	$(TAR) -cf $@ -C $(REP_DIR)/recipes/raw/vbox_files $(FILES)
