#ifndef SCRIPTING_H
#define SCRIPTING_H

uint8_t *script_load_hfs_file(int part, char *path, uint32_t *size);
uint8_t *script_load_file(char *id, uint32_t *size);
char **script_split_file(char *_data, uint32_t _sz, uint32_t *_count);
int script_run_command(char* command);
int script_run_commands(char** cmds, uint32_t count);
int script_run_file(char *path);

#endif
