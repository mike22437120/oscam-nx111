//FIXME Not checked on threadsafety yet; after checking please remove this line

#include "globals.h"

#include "oscam-conf.h"
#include "oscam-conf-chk.h"
#include "oscam-config.h"
#include "oscam-files.h"
#include "oscam-garbage.h"
#include "oscam-lock.h"
#include "oscam-string.h"
#include "oscam-time.h"

extern uint16_t len4caid[256];

#define cs_srid				"oscam.srvid"
#define cs_trid				"oscam.tiers"
#define cs_l4ca				"oscam.guess"
#define cs_sidt				"oscam.services"
#define cs_whitelist		"oscam.whitelist"
#define cs_provid			"oscam.provid"

extern  uint8_t cs_http_use_utf8;

uint32_t cfg_sidtab_generation = 1;

extern char cs_confdir[];

char *get_config_filename(char *dest, size_t destlen, const char *filename) {
	// cs_confdir is always terminated with /
	snprintf(dest, destlen, "%s%s", cs_confdir, filename);
	return dest;
}

int32_t write_services(void)
{
	int32_t i;
	struct s_sidtab *sidtab = cfg.sidtab;
	char *ptr;
	FILE *f = create_config_file(cs_sidt);
	if (!f)
		return 1;

	while(sidtab != NULL){
		ptr = sidtab->label;
		while (*ptr) {
			if (*ptr == ' ') *ptr = '_';
			ptr++;
		}
		fprintf(f,"[%s]\n", sidtab->label);
		fprintf_conf(f, "caid", "%s", ""); // it should not have \n at the end
		for (i=0; i<sidtab->num_caid; i++){
			if (i==0) fprintf(f,"%04X", sidtab->caid[i]);
			else fprintf(f,",%04X", sidtab->caid[i]);
		}
		fputc((int)'\n', f);
		fprintf_conf(f, "provid", "%s", ""); // it should not have \n at the end
		for (i=0; i<sidtab->num_provid; i++){
			if (i==0) fprintf(f,"%06X", sidtab->provid[i]);
			else fprintf(f,",%06X", sidtab->provid[i]);
		}
		fputc((int)'\n', f);
		fprintf_conf(f, "srvid", "%s", ""); // it should not have \n at the end
		for (i=0; i<sidtab->num_srvid; i++){
			if (i==0) fprintf(f,"%04X", sidtab->srvid[i]);
			else fprintf(f,",%04X", sidtab->srvid[i]);
		}
		fprintf(f,"\n\n");
		sidtab=sidtab->next;
	}

	return flush_config_file(f, cs_sidt);
}

void free_sidtab(struct s_sidtab *ptr)
{
		if (!ptr) return;
		add_garbage(ptr->caid); //no need to check on NULL first, freeing NULL doesnt do anything
		add_garbage(ptr->provid);
		add_garbage(ptr->srvid);
		add_garbage(ptr);
}

static void chk_entry4sidtab(char *value, struct s_sidtab *sidtab, int32_t what)
{
  int32_t i, b;
  char *ptr, *saveptr1 = NULL;
  uint16_t *slist=(uint16_t *) 0;
  uint32_t *llist=(uint32_t *) 0;
  uint32_t caid;
  char buf[strlen(value) + 1];
  cs_strncpy(buf, value, sizeof(buf));
  b=(what==1) ? sizeof(uint32_t) : sizeof(uint16_t);
  for (i=0, ptr=strtok_r(value, ",", &saveptr1); ptr; ptr=strtok_r(NULL, ",", &saveptr1))
  {
    caid=a2i(ptr, b);
    if (!errno) i++;
  }
  //if (!i) return(0);
  if (b==sizeof(uint16_t)){
    if (!cs_malloc(&slist, i * sizeof(uint16_t))) return;
  } else {
    if (!cs_malloc(&llist, i * sizeof(uint32_t))) return;
  }
  cs_strncpy(value, buf, sizeof(buf));
  for (i=0, ptr=strtok_r(value, ",", &saveptr1); ptr; ptr=strtok_r(NULL, ",", &saveptr1))
  {
    caid=a2i(ptr, b);
    if (errno) continue;
    if (b==sizeof(uint16_t))
      slist[i++]=(uint16_t) caid;
    else
      llist[i++]=caid;
  }
  switch (what)
  {
    case 0: add_garbage(sidtab->caid);
    		sidtab->caid=slist;
            sidtab->num_caid=i;
            break;
    case 1: add_garbage(sidtab->provid);
    		sidtab->provid=llist;
            sidtab->num_provid=i;
            break;
    case 2: add_garbage(sidtab->srvid);
    		sidtab->srvid=slist;
            sidtab->num_srvid=i;
            break;
  }
}

void chk_sidtab(char *token, char *value, struct s_sidtab *sidtab)
{
  if (!strcmp(token, "caid")) { chk_entry4sidtab(value, sidtab, 0); return; }
  if (!strcmp(token, "provid")) { chk_entry4sidtab(value, sidtab, 1); return; }
  if (!strcmp(token, "ident")) { chk_entry4sidtab(value, sidtab, 1); return; }
  if (!strcmp(token, "srvid")) { chk_entry4sidtab(value, sidtab, 2); return; }
  if (token[0] != '#')
    fprintf(stderr, "Warning: keyword '%s' in sidtab section not recognized\n",token);
}

void init_free_sidtab(void) {
		struct s_sidtab *nxt, *ptr = cfg.sidtab;
		while (ptr) {
				nxt = ptr->next;
				free_sidtab(ptr);
				ptr = nxt;
		}
		cfg.sidtab = NULL;
		++cfg_sidtab_generation;
}

#ifdef DEBUG_SIDTAB
static void show_sidtab(struct s_sidtab *sidtab)
{
  for (; sidtab; sidtab=sidtab->next)
  {
    int32_t i;
    char buf[1024];
    char *saveptr = buf;
    cs_log("label=%s", sidtab->label);
    snprintf(buf, sizeof(buf), "caid(%d)=", sidtab->num_caid);
    for (i=0; i<sidtab->num_caid; i++)
      snprintf(buf+strlen(buf), 1024-(buf-saveptr), "%04X ", sidtab->caid[i]);
    cs_log("%s", buf);
    snprintf(buf, sizeof(buf), "provider(%d)=", sidtab->num_provid);
    for (i=0; i<sidtab->num_provid; i++)
      snprintf(buf+strlen(buf), 1024-(buf-saveptr), "%08X ", sidtab->provid[i]);
    cs_log("%s", buf);
    snprintf(buf, sizeof(buf), "services(%d)=", sidtab->num_srvid);
    for (i=0; i<sidtab->num_srvid; i++)
      snprintf(buf+strlen(buf), 1024-(buf-saveptr), "%04X ", sidtab->srvid[i]);
    cs_log("%s", buf);
  }
}
#endif

int32_t init_sidtab(void) {
	FILE *fp = open_config_file(cs_sidt);
	if (!fp)
		return 1;

	int32_t nr, nro, nrr;
	char *value, *token;
	if (!cs_malloc(&token, MAXLINESIZE))
		return 1;
	struct s_sidtab *ptr;
	struct s_sidtab *sidtab=(struct s_sidtab *)0;

	for (nro=0, ptr=cfg.sidtab; ptr; nro++)
	{
		struct s_sidtab *ptr_next;
		ptr_next=ptr->next;
		free_sidtab(ptr);
		ptr=ptr_next;
	}
	nr = 0; nrr = 0;
	while (fgets(token, MAXLINESIZE, fp))
	{
		int32_t l;
		if ((l=strlen(trim(token)))<3) continue;
		if ((token[0]=='[') && (token[l-1]==']'))
		{
			token[l-1]=0;
			if(nr > MAX_SIDBITS){
				fprintf(stderr, "Warning: Service No.%d - '%s' ignored. Max allowed Services %d\n", nr, strtolower(token+1), MAX_SIDBITS);
				nr++;
				nrr++;
			} else {
				if (!cs_malloc(&ptr, sizeof(struct s_sidtab))) {
					free(token);
					return(1);
				}
				if (sidtab)
					sidtab->next=ptr;
				else
					cfg.sidtab=ptr;
				sidtab=ptr;
				nr++;
				cs_strncpy(sidtab->label, strtolower(token+1), sizeof(sidtab->label));
				continue;
			}
		}
		if (!sidtab) continue;
		if (!(value=strchr(token, '='))) continue;
		*value++='\0';
		chk_sidtab(trim(strtolower(token)), trim(strtolower(value)), sidtab);
	}
	free(token);
	fclose(fp);

#ifdef DEBUG_SIDTAB
	show_sidtab(cfg.sidtab);
#endif
	++cfg_sidtab_generation;
	cs_log("services reloaded: %d services freed, %d services loaded, rejected %d", nro, nr, nrr);
	return(0);
}

//Todo #ifdef CCCAM
int32_t init_provid(void) {
	FILE *fp = open_config_file(cs_provid);
	if (!fp)
		return 0;

	int32_t nr;
	char *payload, *saveptr1 = NULL, *token;
	if (!cs_malloc(&token, MAXLINESIZE))
		return 0;
	static struct s_provid *provid=(struct s_provid *)0;

	nr=0;
	while (fgets(token, MAXLINESIZE, fp)) {

		int32_t l;
		void *ptr;
		char *tmp, *providasc;
		tmp = trim(token);

		if (tmp[0] == '#') continue;
		if ((l = strlen(tmp)) < 11) continue;
		if (!(payload = strchr(token, '|'))) continue;
		if (!(providasc = strchr(token, ':'))) continue;

		*payload++ = '\0';

		if (!cs_malloc(&ptr, sizeof(struct s_provid))) {
			free(token);
			fclose(fp);
			return(1);
		}
		if (provid)
			provid->next = ptr;
		else
			cfg.provid = ptr;

		provid = ptr;

		int32_t i;
		char *ptr1;
		for (i = 0, ptr1 = strtok_r(payload, "|", &saveptr1); ptr1; ptr1 = strtok_r(NULL, "|", &saveptr1), i++){
			switch(i){
			case 0:
				cs_strncpy(provid->prov, trim(ptr1), sizeof(provid->prov));
				break;
			case 1:
				cs_strncpy(provid->sat, trim(ptr1), sizeof(provid->sat));
				break;
			case 2:
				cs_strncpy(provid->lang, trim(ptr1), sizeof(provid->lang));
				break;
			}
		}

		*providasc++ = '\0';
		provid->provid = a2i(providasc, 3);
		provid->caid = a2i(token, 3);
		nr++;
	}
	free(token);
	fclose(fp);
	if (nr>0)
		cs_log("%d provid's loaded", nr);
	return(0);
}

int32_t init_srvid(void)
{
	FILE *fp = open_config_file(cs_srid);
	if (!fp)
		return 0;

	int32_t nr = 0, i;
	char *payload, *tmp, *saveptr1 = NULL, *token;
	if (!cs_malloc(&token, MAXLINESIZE))
		return 0;
	struct s_srvid *srvid=NULL, *new_cfg_srvid[16], *last_srvid[16];
	// A cache for strings within srvids. A checksum is calculated which is the start point in the array (some kind of primitive hash algo).
	// From this point, a sequential search is done. This greatly reduces the amount of string comparisons.
	char **stringcache[1024];
	int32_t allocated[1024] = { 0 };
	int32_t used[1024] = { 0 };
	struct timeb ts, te;
  cs_ftime(&ts);

	memset(last_srvid, 0, sizeof(last_srvid));
	memset(new_cfg_srvid, 0, sizeof(new_cfg_srvid));

	while (fgets(token, MAXLINESIZE, fp)) {
		int32_t l, j, len=0, len2, srvidtmp;
		uint32_t pos;
		char *srvidasc;
		tmp = trim(token);

		if (tmp[0] == '#') continue;
		if ((l=strlen(tmp)) < 6) continue;
		if (!(srvidasc = strchr(token, ':'))) continue;
		if (!(payload=strchr(token, '|'))) continue;
		*payload++ = '\0';

		if (!cs_malloc(&srvid, sizeof(struct s_srvid))) {
			free(token);
			fclose(fp);
			return(1);
		}

		char tmptxt[128];

		int32_t offset[4] = { -1, -1, -1, -1 };
		char *ptr1, *searchptr[4] = { NULL, NULL, NULL, NULL };
		char **ptrs[4] = { &srvid->prov, &srvid->name, &srvid->type, &srvid->desc };

		for (i = 0, ptr1 = strtok_r(payload, "|", &saveptr1); ptr1 && (i < 4) ; ptr1 = strtok_r(NULL, "|", &saveptr1), ++i){
			// check if string is in cache
			len2 = strlen(ptr1);
			pos = 0;
			for(j = 0; j < len2; ++j) pos += (uint8_t)ptr1[j];
			pos = pos%1024;
			for(j = 0; j < used[pos]; ++j){
				if (!strcmp(stringcache[pos][j], ptr1)){
					searchptr[i]=stringcache[pos][j];
					break;
				}
			}
			if (searchptr[i]) continue;

			offset[i]=len;
			cs_strncpy(tmptxt+len, trim(ptr1), sizeof(tmptxt)-len);
			len+=strlen(ptr1)+1;
		}

		char *tmpptr = NULL;
		if (len > 0 && !cs_malloc(&tmpptr, len))
			continue;

		srvid->data=tmpptr;
		if(len > 0) memcpy(tmpptr, tmptxt, len);

		for (i=0;i<4;i++) {
			if (searchptr[i]) {
				*ptrs[i] = searchptr[i];
				continue;
			}
			if (offset[i]>-1){
				*ptrs[i] = tmpptr + offset[i];
				// store string in stringcache
				tmp = *ptrs[i];
				len2 = strlen(tmp);
				pos = 0;
				for(j = 0; j < len2; ++j) pos += (uint8_t)tmp[j];
				pos = pos%1024;
				if(used[pos] >= allocated[pos]){
					if (allocated[pos] == 0) {
						if (!cs_malloc(&stringcache[pos], 16 * sizeof(char *)))
							break;
					} else {
						if (!cs_realloc(&stringcache[pos], (allocated[pos] + 16) * sizeof(char *)))
							break;
					}
					allocated[pos] += 16;
				}
				stringcache[pos][used[pos]] = tmp;
				used[pos] += 1;
			}
		}

		*srvidasc++ = '\0';
		srvidtmp = dyn_word_atob(srvidasc) & 0xFFFF;
		//printf("srvid %s - %d\n",srvidasc,srvid->srvid );

		if (srvidtmp<0) {
			free(tmpptr);
			free(srvid);
			continue;
		} else srvid->srvid = srvidtmp;

		srvid->ncaid = 0;
		for (i = 0, ptr1 = strtok_r(token, ",", &saveptr1); (ptr1) && (i < 10) ; ptr1 = strtok_r(NULL, ",", &saveptr1), i++){
			srvid->caid[i] = dyn_word_atob(ptr1);
			srvid->ncaid = i+1;
			//cs_debug_mask(D_CLIENT, "ld caid: %04X srvid: %04X Prov: %s Chan: %s",srvid->caid[i],srvid->srvid,srvid->prov,srvid->name);
		}
		nr++;

		if (new_cfg_srvid[srvid->srvid>>12])
			last_srvid[srvid->srvid>>12]->next = srvid;
		else
			new_cfg_srvid[srvid->srvid>>12] = srvid;

		last_srvid[srvid->srvid>>12] = srvid;
	}
	for(i = 0; i < 1024; ++i){
		if(allocated[i] > 0) free(stringcache[i]);
	}
	free(token);

	cs_ftime(&te);
	int32_t load_time = 1000*(te.time-ts.time)+te.millitm-ts.millitm;

	fclose(fp);
	if (nr > 0) {
		cs_log("%d service-id's loaded in %dms", nr, load_time);
		if (nr > 2000) {
			cs_log("WARNING: You risk high CPU load and high ECM times with more than 2000 service-id's!");
			cs_log("HINT: --> use optimized lists from http://www.streamboard.tv/wiki/Srvid");
		}
	}

	cs_writelock(&config_lock);
	//this allows reloading of srvids, so cleanup of old data is needed:
	memcpy(last_srvid, cfg.srvid, sizeof(last_srvid));	//old data
	memcpy(cfg.srvid, new_cfg_srvid, sizeof(last_srvid));	//assign after loading, so everything is in memory

	cs_writeunlock(&config_lock);
	
	struct s_client *cl;
	for (cl=first_client->next; cl ; cl=cl->next)
		cl->last_srvidptr=NULL;

	struct s_srvid *ptr;
	for (i=0; i<16; i++) {
		ptr = last_srvid[i];
		while (ptr) { //cleanup old data:			
			add_garbage(ptr->data);
			add_garbage(ptr);
			ptr = ptr->next;
		}
	}

	return(0);
}

int32_t init_tierid(void)
{
	FILE *fp = open_config_file(cs_trid);
	if (!fp)
		return 0;

	int32_t nr;
	char *payload, *saveptr1 = NULL, *token;
	if (!cs_malloc(&token, MAXLINESIZE))
		return 0;
	static struct s_tierid *tierid=NULL, *new_cfg_tierid=NULL;

	nr=0;
	while (fgets(token, MAXLINESIZE, fp)) {

		int32_t l;
		void *ptr;
		char *tmp, *tieridasc;
		tmp = trim(token);

		if (tmp[0] == '#') continue;
		if ((l=strlen(tmp)) < 6) continue;
		if (!(payload=strchr(token, '|'))) continue;
		if (!(tieridasc = strchr(token, ':'))) continue;
		*payload++ = '\0';

		if (!cs_malloc(&ptr,sizeof(struct s_tierid))) {
			free(token);
			fclose(fp);
			return(1);
		}
		if (tierid)
			tierid->next = ptr;
		else
			new_cfg_tierid = ptr;

		tierid = ptr;

		int32_t i;
		char *ptr1 = strtok_r(payload, "|", &saveptr1);
		if (ptr1)
			cs_strncpy(tierid->name, trim(ptr1), sizeof(tierid->name));

		*tieridasc++ = '\0';
		tierid->tierid = dyn_word_atob(tieridasc);
		//printf("tierid %s - %d\n",tieridasc,tierid->tierid );

		tierid->ncaid = 0;
		for (i = 0, ptr1 = strtok_r(token, ",", &saveptr1); (ptr1) && (i < 10) ; ptr1 = strtok_r(NULL, ",", &saveptr1), i++){
			tierid->caid[i] = dyn_word_atob(ptr1);
			tierid->ncaid = i+1;
			// cs_log("ld caid: %04X tierid: %04X name: %s",tierid->caid[i],tierid->tierid,tierid->name);
		}
		nr++;
	}
	free(token);
	fclose(fp);
	if (nr>0)
		cs_log("%d tier-id's loaded", nr);
	cs_writelock(&config_lock);
	//reload function:
	tierid = cfg.tierid;
	cfg.tierid = new_cfg_tierid;
	struct s_tierid *ptr;
	while (tierid) {
		ptr = tierid->next;
		free(tierid);
		tierid = ptr;
	}
	cs_writeunlock(&config_lock);

	return(0);
}

int32_t match_whitelist(ECM_REQUEST *er, struct s_global_whitelist *entry) {
	return ((!entry->caid || entry->caid == er->caid)
			&& (!entry->provid || entry->provid == er->prid)
			&& (!entry->srvid || entry->srvid == er->srvid)
			&& (!entry->chid || entry->chid == er->chid)
			&& (!entry->pid || entry->pid == er->pid)
			&& (!entry->ecmlen || entry->ecmlen == er->ecmlen));
}

int32_t chk_global_whitelist(ECM_REQUEST *er, uint32_t *line)
{
	*line = -1;
	if (!cfg.global_whitelist)
		return 1;

	struct s_global_whitelist *entry;

	//check mapping:
	if (cfg.global_whitelist_use_m) {
		entry = cfg.global_whitelist;
		while (entry) {
			if (entry->type == 'm') {
				if (match_whitelist(er, entry)) {
					er->caid = entry->mapcaid;
					er->prid = entry->mapprovid;
					cs_debug_mask(D_TRACE, "whitelist: mapped %04X:%06X to %04X:%06X", er->caid, er->prid, entry->mapcaid, entry->mapprovid);
					break;
				}
			}
			entry = entry->next;
		}
	}

	if (cfg.global_whitelist_use_l) { //Check caid/prov/srvid etc matching, except ecm-len:
		entry = cfg.global_whitelist;
		int8_t caidprov_matches = 0;
		while (entry) {
			if (entry->type == 'l') {
				if (match_whitelist(er, entry)) {
					*line = entry->line;
					return 1;
				}
				if ((!entry->caid || entry->caid == er->caid)
						&& (!entry->provid || entry->provid == er->prid)
						&& (!entry->srvid || entry->srvid == er->srvid)
						&& (!entry->chid || entry->chid == er->chid)
						&& (!entry->pid || entry->pid == er->pid)){
					caidprov_matches = 1;
					*line = entry->line;
				}
			}
			entry = entry->next;
		}
		if (caidprov_matches) //...but not ecm-len!
			return 0;
	}

	entry = cfg.global_whitelist;
	while (entry) {
		if (match_whitelist(er, entry)) {
			*line = entry->line;
			if (entry->type == 'w')
				return 1;
			else if (entry->type == 'i')
				return 0;
		}
		entry = entry->next;
	}
	return 0;
}

//Format:
//Whitelist-Entry:
//w:caid:prov:srvid:pid:chid:ecmlen
//Ignore-Entry:
//i:caid:prov:srvid:pid:chid:ecmlen
//ECM len check - Entry:
//l:caid:prov:srvid:pid:chid:ecmlen

//Mapping:
//m:caid:prov:srvid:pid:chid:ecmlen caidto:provto

static struct s_global_whitelist *global_whitelist_read_int(void) {
	FILE *fp = open_config_file(cs_whitelist);
	if (!fp)
		return NULL;

	char token[1024], str1[1024];
	unsigned char type;
	int32_t i, ret, count=0;
	struct s_global_whitelist *new_whitelist = NULL, *entry, *last=NULL;
	uint32_t line = 0;

	cfg.global_whitelist_use_l = 0;
	cfg.global_whitelist_use_m = 0;

	while (fgets(token, sizeof(token), fp)) {
		line++;
		if (strlen(token) <= 1) continue;
		if (token[0]=='#' || token[0]=='/') continue;
		if (strlen(token)>1024) continue;

		for (i=0;i<(int)strlen(token);i++) {
			if ((token[i]==':' || token[i]==' ') && token[i+1]==':') {
				memmove(token+i+2, token+i+1, strlen(token)-i+1);
				token[i+1]='0';
			}
			if (token[i]=='#' || token[i]=='/') {
				token[i]='\0';
				break;
			}
		}

		type = 'w';
		uint32_t caid=0, provid=0, srvid=0, pid=0, chid=0, ecmlen=0, mapcaid=0, mapprovid=0;
		memset(str1, 0, sizeof(str1));

		ret = sscanf(token, "%c:%4x:%6x:%4x:%4x:%4x:%1023s", &type, &caid, &provid, &srvid, &pid, &chid, str1);

		type = tolower(type);

		//w=whitelist
		//i=ignore
		//l=len-check
		//m=map caid/prov
		if (ret < 1 || (type != 'w' && type != 'i' && type != 'l' && type != 'm'))
			continue;

		if (type == 'm') {
			char *p = strstr(token+4, " ");
			if (!p || sscanf(p+1, "%4x:%6x", &mapcaid, &mapprovid) < 2) {
				cs_debug_mask(D_TRACE, "whitelist: wrong mapping: %s", token);
				continue;
			}
			str1[0]=0;
			cfg.global_whitelist_use_m = 1;
		}
		strncat(str1, ",", sizeof(str1) - strlen(str1) - 1);
		char *p = str1, *p2 = str1;
		while (*p) {
			if (*p == ',') {
				*p = 0;
				ecmlen = 0;
				sscanf(p2, "%4x", &ecmlen);

				if (!cs_malloc(&entry, sizeof(struct s_global_whitelist))) {
					fclose(fp);
					return new_whitelist;
				}

				count++;
				entry->line = line;
				entry->type = type;
				entry->caid = caid;
				entry->provid = provid;
				entry->srvid = srvid;
				entry->pid = pid;
				entry->chid = chid;
				entry->ecmlen = ecmlen;
				entry->mapcaid = mapcaid;
				entry->mapprovid = mapprovid;
				if (entry->type == 'l')
					cfg.global_whitelist_use_l = 1;

				if (type == 'm')
					cs_debug_mask(D_TRACE,
							"whitelist: %c: %04X:%06X:%04X:%04X:%04X:%02X map to %04X:%06X", entry->type, entry->caid, entry->provid, entry->srvid, entry->pid, entry->chid, entry->ecmlen, entry->mapcaid, entry->mapprovid);
				else
					cs_debug_mask(D_TRACE,
						"whitelist: %c: %04X:%06X:%04X:%04X:%04X:%02X", entry->type, entry->caid, entry->provid, entry->srvid, entry->pid, entry->chid, entry->ecmlen);

				if (!new_whitelist) {
					new_whitelist = entry;
					last = new_whitelist;
				} else {
					last->next = entry;
					last = entry;
				}

				p2 = p + 1;
			}
			p++;
			}
	}

	if (count)
		cs_log("%d entries read from %s", count, cs_whitelist);

	fclose(fp);

	return new_whitelist;
}

void global_whitelist_read(void) {

	struct s_global_whitelist *entry, *old_list;

	old_list = cfg.global_whitelist;
	cfg.global_whitelist = global_whitelist_read_int();

	while (old_list) {
		entry = old_list->next;
		free(old_list);
		old_list = entry;
	}
}

void init_len4caid(void)
{
	FILE *fp = open_config_file(cs_l4ca);
	if (!fp)
		return;

	int32_t nr;
	char *value, *token;

	if (!cs_malloc(&token, MAXLINESIZE))
		return;

	memset(len4caid, 0, sizeof(uint16_t)<<8);
	for(nr = 0; fgets(token, MAXLINESIZE, fp);) {
		int32_t i, c;
		char *ptr;
		if (!(value=strchr(token, ':')))
			continue;
		*value++ ='\0';
		if( (ptr = strchr(value, '#')) )
			*ptr = '\0';
		if (strlen(trim(token)) != 2)
			continue;
		if (strlen(trim(value)) != 4)
			continue;
		if ((i = byte_atob(token)) < 0)
			continue;
		if ((c = word_atob(value)) < 0)
			continue;
		len4caid[i] = c;
		nr++;
	}
	free(token);
	fclose(fp);
	if (nr)
		cs_log("%d lengths for caid guessing loaded", nr);
	return;
}

int32_t chk_cccam_cfg_F_more(char *line,struct s_auth * account)
{
	int32_t p1=0,off=0,no=0,cno=0,dno=0,reshare=0;
	char *p=NULL,*sline=NULL,*p2=NULL,*p3=NULL;
	char *optr=NULL,*iptr=NULL;
	for(p=line;*p;p++){
		if(*p=='{'){
			p1=1;
			sline=p+1;
			off=0;
			continue;
		}
		
		if(*p=='}' && p1==1){
			sline[off]='\0';
//			cs_debug_mask(D_TRACE,"parase CCcam.cfg F line:part{%s}",sline);
			p1=0;
			no++;
			cno=0;
			while((p2=strtok_r(sline,",",&optr))){
				dno=0;
				uint16_t caid=0,srvid=0;
				uint32_t provid=0;
//				cs_debug_mask(D_TRACE,"CHILD:%s",p2);
				while((p3=strtok_r(p2,":",&iptr))){
					if(!dno){
						caid=(uint16_t)a2i(p3,4);
						account->ftab.filts[dno].caid=caid;
					}
					if(dno==1){
						provid=(uint32_t)a2i(p3,6);
						account->ftab.filts[dno].prids[account->ftab.filts[dno].nprids]=provid;
						account->ftab.filts[dno].nprids++;
					}
					if(dno==2){
						if(!cno && !(account->cccreshare)){
							sscanf(p3,"%d",&reshare);
							account->cccreshare=(int32_t)reshare;
						}
						if(cno==1){
							srvid=(uint16_t)a2i(p3,4);

							struct s_sidtab *sp,*sidtab=cfg.sidtab;
							uint32_t sppos=0;
							for(sp=cfg.sidtab;sp;sp=sp->next,sppos++){
								uint32_t j,found;

								sidtab=sp;
								for(j=0,found=0;!found && j<sp->num_caid;j++)
									if(sp->caid[j]==caid)
										found=1;
								if(!found)continue;
								
								for(j=0,found=0;!found && j<sp->num_provid;j++)
									if(sp->provid[j]==provid)
										found=1;
								if(!found)continue;

								for(j=0,found=0;!found && j<sp->num_srvid;j++)
									if(sp->srvid[j]==srvid)
										found=1;
								if(!found)continue;

							}
							if(sp){
								account->sidtabs.ok |= (1<<sppos);
								continue;
							}

							if (!cs_malloc(&sp, sizeof(struct s_sidtab)) || !sp)continue;
							if (sidtab)
								sidtab->next=sp;
						      	else	
								cfg.sidtab=sp;
						      	sppos++;
						      	memset(sp, 0, sizeof(struct s_sidtab));
						      	snprintf(sp->label, sizeof(sp->label),"%04x_%06X_%04X",caid,provid,srvid);
							char scaid[5],sprovid[7],ssrvid[5];
							snprintf(scaid,sizeof(scaid),"%04X",caid);
							snprintf(sprovid,sizeof(sprovid),"%04X",provid);
							snprintf(scaid,sizeof(scaid),"%04X",provid);

						    	chk_sidtab("caid",scaid, sp);
						    	chk_sidtab("provid",sprovid,sp);
						    	chk_sidtab("caid",ssrvid, sp);
							account->sidtabs.ok |= (1<<sppos);
						}
					}
					dno++;
					p2=NULL;
				}
				cno++;
				sline=NULL;
			}
			
			continue;
		}
		off++;
	}
	return 0;
}

void * read_cccamcfg(int32_t mode)
{
	struct s_auth *authptr = NULL;
	FILE *fp;
	char token[MAXLINESIZE];
	char line[MAXLINESIZE];
	char host[256],uname[128],upass[128],uhops[128],uemu[128],uemm[128];
	char typ;
	static int32_t readed_cccamcfg=0;
	int32_t port,ret;
	int32_t caid,prid;
	
	if(!readed_cccamcfg)
		cs_log("load CCcam config file: %s",cfg.cc_cfgfile);

	if(!cfg.cc_cfgfile || (mode != CCCAMCFGREADER && mode != CCCAMCFGUSER))
			return NULL;

	readed_cccamcfg=1;

	if(!(fp=fopen(cfg.cc_cfgfile,"r"))){
		cs_log("can't open file \"%s\" (errno=%d)\n", cfg.cc_cfgfile, errno);
		return NULL;
	}

	struct s_auth *account=NULL;
	struct s_reader *rdr;

	while (fgets(token,sizeof(token),fp)) {
		void *ptr;
		char *p=strchr(token,'#');
		if(p)
			*p='\0';
		strncpy(line,trim(token),MAXLINESIZE-1);
		if(!line[0])continue;
		if((line[0] == 'C' || line[0] == 'L' || line[0] == 'N' || line[0] == 'R' ) && line[1] == ':' && (mode == CCCAMCFGREADER)){

			int32_t paracount=0;
			char * proto=0;
			ret=0;
			uchar ncd_key[13+sizeof(uint)];
			memset(ncd_key,0,sizeof(ncd_key));
			int32_t reshare=-1;
			switch(line[0]){
				case 'C':
					proto = "cccam";
					ret=sscanf(line,"%c:%s%d%s%s",&typ,host,&port,uname,upass);
					paracount=5;
					break;
				case 'L':
					proto = "camd35";
					ret=sscanf(line,"%c:%s%d%s%s%x%x%d",&typ,host,&port,uname,upass,&caid,&prid,&reshare);
					paracount=5;
					break;
				case 'N':
					proto = "newcamd";
					ret=sscanf(line,"%c:%s%d%s%s%x%x%x%x%x%x%x%x%x%x%x%x%x%x%d",&typ,host,&port,uname,upass,
						(uint*) &ncd_key[0], (uint*) &ncd_key[1], (uint*) &ncd_key[2], (uint*) &ncd_key[3],(uint*) &ncd_key[4],
						(uint*) &ncd_key[5], (uint*) &ncd_key[6], (uint*) &ncd_key[7], (uint*) &ncd_key[8],(uint*) &ncd_key[9],
						(uint*) &ncd_key[10],(uint*) &ncd_key[11],(uint*) &ncd_key[12],(uint*) &ncd_key[13],&reshare);
					paracount=5;
					break;
				case 'R':
					proto = "radegast";
					ret=sscanf(line,"%c:%s%d%x%x%d",&typ,host,&port,&caid,&prid,&reshare);
					paracount=3;
					break;
			}

			if(!proto || ret<paracount)continue;

			int32_t found=0;
			LL_ITER itr = ll_iter_create(configured_readers);
			struct s_reader *prdr=NULL;
			while((prdr = ll_iter_next(&itr))){
				if( strcasecmp(prdr->device,host) == 0 && prdr->r_port == port &&
				    strcmp(prdr->r_usr,uname) == 0  && strcmp(prdr->r_pwd,upass) == 0 &&
				    host[0] && port && uname[0] && upass[0] ){
					found=1;
					break;
				}
			}
			if(found)
				continue;

			if(!cs_malloc(&rdr,sizeof(struct s_reader)))
					continue;

			memset(rdr, 0, sizeof(struct s_reader));

			reader_set_defaults(rdr);

			chk_reader("protocol",proto,rdr);
			cs_strncpy(rdr->device,host,sizeof(rdr->device));
			rdr->r_port = port;
			cs_strncpy(rdr->r_usr,uname,sizeof(rdr->r_usr));
			cs_strncpy(rdr->r_pwd,upass,sizeof(rdr->r_pwd));
			snprintf(token,sizeof(token),"%s_%d",host,port);
			cs_strncpy(rdr->label,token,sizeof(rdr->label));
			rdr->grp = 1;	

			ll_append(configured_readers, rdr);
			cs_debug_mask(D_READER,"Add reader device=%s,%d(type:0x%X,protocol=%s)from CCcam.cfg",rdr->device,rdr->r_port,rdr->typ,proto);
		}
		else if (line[0]=='F' && line[1]==':' && mode==CCCAMCFGUSER){
			ret=sscanf(line,"F:%126s%126s%126s%126s%126s",uname,upass,uhops,uemu,uemm);
			if(ret<2)continue;
			if(ret<5)cs_strncpy(uemm,"1",sizeof(uemm));
			if(ret<4)cs_strncpy(uemu,"1",sizeof(uemu));
			if(ret<3)cs_strncpy(uhops,"10",sizeof(uhops));
			
			//cs_log("Read Line:%s",line);
			int32_t found=0;
			struct s_auth *pusr=NULL;
			for(pusr=authptr;pusr;pusr=pusr->next){
				if(!strcmp(pusr->usr,uname)){
					found=1;
					break;
				}
			}
			if(found)
				continue;

			if(!cs_malloc(&ptr, sizeof(struct s_auth))) return (void *)authptr;
			if (account)
				account->next = ptr;
			else	
				authptr = ptr;

			account = ptr;
			account_set_defaults(account);

			chk_account("user",uname,account);
			chk_account("pwd",upass,account);
			chk_account("cccmaxhops",uhops,account);
			chk_account("au",uemu,account);
			chk_cccam_cfg_F_more(line,account);
			cs_debug_mask(D_TRACE,"Add usr: %s from CCcam.cfg",account->usr);

		}
	}
	fclose(fp);
	if(mode == CCCAMCFGREADER)
		return (void *)configured_readers;
	else if(mode == CCCAMCFGUSER)
		return (void *)authptr;

	return NULL;
}

