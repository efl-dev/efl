edc_files = [
  'test.edc',
  'test_external.edc',
  'test_focus_style.edc',
  'test_masking.edc',
  'test_tab_pager.edc',
  'test_tooltip.edc',
  'test_pager.edc',
  'test_prefs.edc',
  'cursors.edc',
  'font_preview.edc',
  'multibuttonentry.edc',
  'multip.edc',
  'postit_ent.edc'
]

elm_themes = []

foreach edc_file : edc_files
   elm_themes += custom_target('edje_cc_elm_' + edc_file,
    input : edc_file,
    output : '@BASENAME@.edj',
    command : edje_cc_exe + [ '-beta', '-fastcomp',
              '-no-warn-unused-images',
              '-sd', elm_themes_sound_include,
              '-id', elm_themes_image_include,
              '-id', join_paths(meson.current_source_dir()),
              '-fd', join_paths(meson.current_source_dir()),
              '@INPUT@', '@OUTPUT@'],
    depends : edje_depends,
    install : true,
    install_dir : join_paths(dir_data, 'elementary', 'objects'),
   )
endforeach

custom_target('prefs_compile',
  input: 'test_prefs.epc',
  output: 'test_prefs.epb',
  command : elm_prefs_cc_exe + ['@INPUT@', '@OUTPUT@'],
    depends : elm_prefs_cc,
    install : true,
    install_dir : join_paths(dir_data, 'elementary', 'objects'),
)
