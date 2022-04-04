project (
	'blz77',
	'c',
	version: '0.1',
	default_options: [
		'warning_level=3',
		'c_std=gnu99',
	]
)

# Configure `blz77_version.h`.
conf_data = configuration_data ()
conf_data.set ('BLZ77_VERSION', meson.project_version ())
configure_file (
	input: 'blz77_version.h.in',
	output: 'blz77_version.h',
	configuration: conf_data,
)

## Library versioning
##
## New package version:
##   revision += 1
##
## New interfaces:
##   current += 1
##   revision = 0
##   age += 1
##
## Deleted/changed interfaces:
##   current += 1
##   revision = 0
##   age = 0
##
## See: <https://gitlab.com/bzip2/bzip2/-/blob/master/meson.build>
blz77_lt_current  = 0
blz77_lt_revision = 0
blz77_lt_age      = 0

blz77_soversion = blz77_lt_current - blz77_lt_age
blz77_lt_version = '@0@.@1@.@2@'.format (blz77_soversion, blz77_lt_age, blz77_lt_revision)

libblz77 = library (
	'blz77',
	'libblz77.c',
	version: blz77_lt_version,
	soversion: blz77_soversion,
	install: true,
)

executable (
	'blz77',
	'blz77.c',
	link_with: libblz77,
	install: true,
)
