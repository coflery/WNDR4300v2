/***
 * Add this file to support multi pppoe function.
 *
 */

#include "dnsmasq.h"

//ifdef SUP_MUL_PPPOE
static int check_dname_wildcard(struct dname_record *drecord)
{
  char *dnamep = drecord->dname;
  char *buf[128] = {0};
  if (drecord == NULL && dnamep == NULL)
  	return -1;
  if (*dnamep == '*'){
  	drecord->wildcard = 1;
	strcpy(buf, dnamep+1);
	strcpy(dnamep, buf);
  }
  else{
  	drecord->wildcard = 0;
  }
  return 0;
}

static int load_mul_pppoe_config(char *filename)
{
  FILE *fp = NULL;
  char buf[128] = {0};
  char *sp = " \t\n\t";
  struct dname_record *tmp_rc = NULL;
  struct dname_record *next_rc = NULL;
  if (NULL == (fp = fopen(filename, "r"))) {
      goto err;
  }
  if (dname_list != NULL)
  	for (tmp_rc = dname_list; tmp_rc; tmp_rc = next_rc) {
		next_rc = tmp_rc->next;
		free(tmp_rc);
	}
  dname_list = NULL;
  while (fgets(buf, sizeof(buf), fp)) {
      char *name = strtok(buf, sp);
      if (NULL == name)
         continue;
      tmp_rc = safe_malloc(sizeof(struct dname_record));
      strcpy(tmp_rc->dname, name);
      check_dname_wildcard(tmp_rc);
      my_syslog(LOG_INFO, _("mul_pppoe_config, dname: %s wildcard:%d"), tmp_rc->dname, tmp_rc->wildcard);
      tmp_rc->next = dname_list;
      dname_list = tmp_rc;
  }
  system("/sbin/mul_pppoe_dns clear_all_record");
  fclose(fp);
  return 0;

err:
  if (fp)
     fclose(fp);
  return -1;
}

static int inc_check_list( char *dname)
{
	struct dname_record *tmp_rc = NULL;
	char *tmpr = NULL;
	if (dname_list == NULL && dname == NULL)
		return -1;
	for (tmp_rc = dname_list; tmp_rc; tmp_rc = tmp_rc->next) {
		if (tmp_rc->wildcard == 0){
			if (tmp_rc->dname != NULL && 0 == strcmp(tmp_rc->dname, dname))
				return 0;
		}
		else{
			if (tmp_rc->dname != NULL && (tmpr = strstr(dname, tmp_rc->dname)) != NULL)
				if (0 == strcmp(tmp_rc->dname, tmpr))
					return 0;
		}
	}
	return -1;
}

static void update_config_check(void)
{
	struct stat statbuf_mp;
	if (NULL != dname_check_file && -1 != stat(dname_check_file, &statbuf_mp)) {
		if (statbuf_mp.st_mtime != dname_save_time) {
			if (0 == load_mul_pppoe_config(dname_check_file))
				dname_save_time = statbuf_mp.st_mtime;
		}
	}
}

static int mul_pppoe_function_check(void)
{
	FILE *fp = NULL;
	if((fp = fopen("/etc/ppp/enable_ppp1", "r")) == NULL ){
		my_syslog(LOG_INFO, "ppp1 not enable, return");
		return -1;
	}
	fclose(fp);
	return 0;
}

void check_mul_pppoe_record(HEADER *header, int plen)
{
  unsigned char dname[MAXDNAME] = {0};
  unsigned short int qtype;
  int addrcount = 0;
  int i;
  struct in_addr i_addr[128];
  char *cmd[128] = {'\0'};

  if (mul_pppoe_function_check() == -1)
   return;

  if (NULL == header)
   return;
  if (header->opcode != QUERY || header->rcode != NOERROR)
   return;
  /* it must only one query in this packet*/
  if (ntohs(header->qdcount) != 1)
   return;
  /* it's not answer in this dns reply packet */
  if (ntohs(header->ancount) <= 0)
   return;
  /* get the query dname and query type */
  if (extract_request(header, plen, dname, &qtype) == 0)
   return;
  if (qtype != T_A)
   return;

  update_config_check();
  if (inc_check_list(dname) == -1)
   return;
  my_syslog(LOG_INFO, "start to get_resolve_address");

  get_resolve_address(&addrcount, i_addr, header, plen);

  my_syslog(LOG_INFO, "get addrcount is %d", addrcount);

  if (addrcount == 0)
   return;
  for (i = 0; i < addrcount; i++)
  {
   sprintf(cmd, "/sbin/mul_pppoe_dns add_record %s %s", dname, inet_ntoa(i_addr[i]));
   system(cmd);
  }
  return;
}
//endif
