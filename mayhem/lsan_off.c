/*
 * mpv/mayhem/lsan_off.c — the sanctioned build-time LeakSanitizer off-switch (SPEC §6.2 item 15).
 *
 * Returning nonzero from __lsan_is_turned_off() turns off ONLY the at-exit leak check; AddressSanitizer
 * and UBSan stay fully active (memory-safety and UB reports still fire and still abort).
 *
 * mayhem/build.sh compiles this with the same $CFLAGS ($SANITIZER_FLAGS + $DEBUG_FLAGS) as the rest of
 * the build and threads the object into meson's c_link_args/cpp_link_args, so it is linked into every
 * executable of the top-level mpv project: all 20 fuzzer_* libFuzzer binaries (fuzzer_loadfile,
 * fuzzer_loadfile_direct, fuzzer_loadfile_mkv, fuzzer_load_config_file, fuzzer_load_input_conf,
 * fuzzer_options_parser, fuzzer_json, fuzzer_protocol_{dvb,edl,file,lavf,memory},
 * fuzzer_set_property_MPV_FORMAT_{STRING,FLAG,INT64,DOUBLE}_{0,1}) plus the sanitized mayhem_kat oracle.
 */
int __lsan_is_turned_off(void) { return 1; }
