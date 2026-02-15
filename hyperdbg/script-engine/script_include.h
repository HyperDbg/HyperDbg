#define MAX_PATH_LEN 1024

void
ResolveIncludePath(const char * IncludeFilePath, char * OutPath);

BOOLEAN
FileExists(const char * path);

BOOLEAN
ParseIncludeFile(char * IncludeFile, char ** Buffer);

char*
InsertStrNew(char* str, int InputIdx, const char* buf);