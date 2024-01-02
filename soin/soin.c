/* soin: inline troff .so requests */
#include <ctype.h>
#include <direct.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#define MAX_PATH _MAX_PATH
#define getcwd getcwd
#endif

#define LNLEN		2048

static char **dirpaths;

static int soin(char *path);

static int soin_cmd(char *s)
{
	char path[LNLEN];
	char *d = path;
	if (s[0] != '.' || s[1] != 's' || s[2] != 'o' || s[3] != ' ')
		return 1;
	s += 3;
	while (isspace((unsigned char) *s))
		s++;
	if (s[0] == '"') {
		s++;
		while (*s && *s != '\n' && *s != '"')
			*d++ = *s++;
	} else {
		while (*s && *s != ' ' && *s != '\n')
			*d++ = *s++;
	}
	*d = '\0';
	return soin(path);
}

/* Search dirpaths[] to open 'filepath'
 * Note complexities with path handling, and O/S dependencies :(
 * General processing rule:
 *   - '-' represents stdin - it can only be used once
 *   - if filepath is absolute don't perform searching
 *   - o/w append filepath to each directory in the search path
       until an accessible file is found
 */
static int usedstdin;
static char fpath[MAX_PATH];

#ifdef _WIN32
/* Traditional format [drive:][\](subpath\)*filename
 *   -- absolute or relative
 *   -- drive letter interpreted as an absolute path
 * UNC format  \\server\(subpath\)*filename
 * DOS device  \\<.?>\[drive:]
 *
 * Note that the MS fopen() implementation performs normalisation,
 * and allows both forward and backslashes as directory separators.
 *
 * A well-formed UNC path MUST use backslashes initially.
 */
static int issep(char c) { return c=='/' || c=='\\'; }

static FILE *fsearch(char *filepath)
{
	if(!filepath) {
		fprintf(stderr, "soin: internal error - NULL filepath\n");
		exit(1);
	}
	if(!filepath[0]) {
		fprintf(stderr, "soin: empty filepath\n");
		exit(1);
	}
	if('-' == filepath[0]) {
		if(usedstdin) {
			fprintf(stderr, "soin: cannot use stdin ('-') twice\n");
			exit(1);
		}
		usedstdin = 1;
		strcpy(fpath,"stdin");
		return stdin;
	}
	else if(issep(filepath[0]) || ':'==filepath[1]) { /* Always absolute */
		//strcpy(fpath,filepath);
		return fopen(filepath, "rt");
	}
	else
	{
		int i,j,k;
		FILE *f;
		for(i=0;dirpaths[i];++i) {
fprintf(stderr,"dirpath[%d]=%s\n",i,dirpaths[i]);
			for(j=0;dirpaths[i][j];++j) fpath[j]=dirpaths[i][j];
			if(!issep(fpath[j-1])) fpath[j++]='\\';
			for(k=0;filepath[k];++k) fpath[j++]=filepath[k];
			fpath[j]='\0';
fprintf(stderr,"Trying: %s\n",fpath);
			if((f=fopen(fpath,"rt"))) return f;
		}
	}
	fprintf(stderr, "soin: cannot open <%s>\n", filepath);
	exit(1);
}
#else
static FILE *fsearch(char *filepath)
{
#error not implemented
	return 0;
}
#endif

static int soin(char *path)
{
	FILE *fp = fsearch(path);
	int lineno = 1;
	char ln[LNLEN];
	/* XXX Note 'path' - not final resolved fpath; also munges 'stdin' */
	printf(".lf %d %s\n", lineno, path);
	while (fgets(ln, sizeof(ln), fp)) {
		lineno++;
		if (!soin_cmd(ln))
			printf(".lf %d %s\n", lineno, path);
		else
			fputs(ln, stdout);
		if (ln[0] == '.' && ln[1] == 'l' && ln[2] == 'f')
			sscanf(ln, ".lf %d", &lineno);
	}
	if(stdin!=fp)
		fclose(fp);
	return 0;
}

/* soin [-I<path>...] [filename...] */
int main(int argc, char **argv)
{
	int i,j;
	dirpaths = calloc(sizeof(char*),argc+1);
	if(!dirpaths){
		fprintf(stderr,"soin: out of memory\n");
		return 1;
	}
	if(!(dirpaths[0] = malloc(MAX_PATH+1))){
		fprintf(stderr,"soin: out of memory\n");
		return 1;
	}
	if(!getcwd(dirpaths[0],MAX_PATH+1)){
		fprintf(stderr,"soin: getcwd() failed\n");
		return 1;
	}
	dirpaths[j = 1] = 0;
	for(i = j = 1; i < argc && argv[i][0] == '-'; ++i) {
		if(argv[i][1] == 'I') {
			if(!argv[i][2]) { /* Separate argument? */
				dirpaths[j++] = argv[++i];
			} else {
				dirpaths[j++] = argv[i]+2;
			}
		} else {
			fprintf(stderr,"soin: unknown argument '-%c...'\n", argv[i][1]);
			return 1;
		}
	}
	dirpaths[j] = 0;

	if(i==argc) soin("-"); /* Force stdin */
	else for(;i<argc;++i) soin(argv[i]);

	free(dirpaths[0]);
	free(dirpaths);
	return 0;
}
