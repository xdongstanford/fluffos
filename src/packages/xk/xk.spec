
void let_us_make_a_crash(void);
string xk_cmdrc4(string);

void stdout(string, ...);
void stderr(string, ...);
float fuptime();
float ftime();
mixed *shuffle_array(mixed *);

string json_stringify(mixed, int default: 0);
mixed json_parse(string);

int pg_close(int);
int pg_ok(int);
int pg_connect(string);
mixed pg_call(int, string, ...);
void pg_send(int, string, ...);
mixed pg_get(int);
string pg_look(void);

string malloc_stats_print(string);

void jemalloc_purge(void);

