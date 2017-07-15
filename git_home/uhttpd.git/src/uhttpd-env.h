#ifndef _UHTTPD_ENV_

struct env_config {
	char *env_name;
	char *conf_name;
	char value[128];
};

void init_product_envs(void);
void set_product_envs(void);

#endif
