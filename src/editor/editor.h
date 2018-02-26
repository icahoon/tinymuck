#define COMMAND_FUNC(func) static void func(const dbref, const dbref, const char **)
COMMAND_FUNC(edit_show_abbrev);
COMMAND_FUNC(edit_compile);
COMMAND_FUNC(edit_delete);
COMMAND_FUNC(edit_help);
COMMAND_FUNC(edit_insert);
COMMAND_FUNC(edit_kill_macro);
COMMAND_FUNC(edit_list);
COMMAND_FUNC(edit_linenos);
COMMAND_FUNC(edit_quit);
COMMAND_FUNC(edit_show_macros);
COMMAND_FUNC(edit_linenos);
COMMAND_FUNC(edit_unassemble);
COMMAND_FUNC(edit_view);
#undef COMMAND_FUNC
