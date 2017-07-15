#ifndef __CONFIG_H_
#define __CONFIG_H_

#define POT_MTD			"/dev/mtd/5"
#define POT_FILENAME		"/tmp/pot_value"

#define POT_MAX_VALUE	4320		/*4320m */
#define POT_RESOLUTION	1		/*60s*/
#define POT_PORT			3333

extern void config_set(char *name, char *value);
extern void config_commit(void);
extern char *config_get(char *name);
extern int config_match(char *name, char *match);
extern int config_invmatch(char *name, char *match);


#define STAMAC_POSTION	(2048 + 4)
#endif
