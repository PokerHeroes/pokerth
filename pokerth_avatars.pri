unix:!mac:!android {
	data_copyright.path += $${PREFIX}/share/pokerth/data/
	data_copyright.files += data/data-copyright.txt

	avatars.depends += install_data_copyright
	avatars.path += $${PREFIX}/share/pokerth/data/gfx/avatars/
	avatars.files += data/gfx/avatars/*

	INSTALLS += data_copyright avatars
}
