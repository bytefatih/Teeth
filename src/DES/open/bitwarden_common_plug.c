/*
 * Common code for the Bitwarden format.
 */

#include "arch.h"
#include "misc.h"
#include "common.h"
#include "bitwarden_common.h"

struct fmt_tests bitwarden_tests[] = {
	// Extracted from Firefox 57.x running on Fedora 27
	{"$bitwarden$0*5000*lulu@mailinator.com*20d9c3c9daaed076026b6cb5887d3273*3bbcb4c7cec45d71c7238291573eb8a8a0f71e6191fb708b07f2cb43b26a56b533ba35a5906abdc08600baedb18fbc042a3b50f4549890210a254129b0ae749394c3c39b33ca183c605ee97b167329d3", "openwall123"},
	{"$bitwarden$0*5000*lulu@mailinator.com*7bb0e633c7e2cf45aa44f4d7607cd6fa*cc785f4d89dd5d1c2647bf9a82edcfe6eb640ff94ba9258110c36cca3f7ccb150a32661683e4a1c18207b03e1586e0b6b2abdad0b3c34983ecd6510e2a80c81a530868fe017f590e6c6656bdcbb634e1", "openwall12345"},
	// Generated by https://help.bitwarden.com/crypto.html
	{"$bitwarden$0*5000*user@example.com*34f9432d563aec619302edf7a8cf5d1e*9ece44d902056ba9eb0e0ad9b2182ab22c812d2939f1feb0c936e4bd81d0c86e114937424f33981715c6ddfb76bd4e24a030d574c2f11ee021a2bbced8a8b937610001d7396dceba363d1711a753227f", "notastrongpassword"},
	{"$bitwarden$0*5000*user@example.com*ce49243f6593589f7fa825411fbeb8d7*d90deb720836c5ea5c91eccc896035e510fc8951606266e840b6d7748a623ef68cf98fb69ec8ab5b0fe6a0ddd48edcda9aa6eef07125d89ea417477b96bfd2db462f2b2363f73b5553d78e8b3f372e3a", "äbc"},
	{NULL}
};

int bitwarden_common_valid(char *ciphertext, struct fmt_main *self)
{
	char *ctcopy, *keeptr, *p;
	int value, extra;

	if (strncmp(ciphertext, FORMAT_TAG, TAG_LENGTH) != 0)
		return 0;

	ctcopy = xstrdup(ciphertext);
	keeptr = ctcopy;

	ctcopy += TAG_LENGTH;
	if ((p = strtokm(ctcopy, "*")) == NULL) // version, for future purposes
		goto err;
	if (!isdec(p))
		goto err;
	value = atoi(p);
	if (value != 0)
		goto err;
	if ((p = strtokm(NULL, "*")) == NULL)   // iterations (fixed)
		goto err;
	if (!isdec(p))
		goto err;
	if (atoi(p) != 5000)
		goto err;
	if ((p = strtokm(NULL, "*")) == NULL)   // salt
		goto err;
	if (strlen(p) > SALTLEN - 1)
		goto err;
	if ((p = strtokm(NULL, "*")) == NULL)   // iv
		goto err;
	if (hexlenl(p, &extra) != 32 || extra)
		goto err;
	if ((p = strtokm(NULL, "*")) == NULL)   // encrypted blob
		goto err;
	if (hexlenl(p, &extra) != BLOBLEN * 2 || extra)
		goto err;

	MEM_FREE(keeptr);
	return 1;

err:
	MEM_FREE(keeptr);
	return 0;
}

void *bitwarden_common_get_salt(char *ciphertext)
{
	char *ctcopy = xstrdup(ciphertext);
	char *keeptr = ctcopy;
	int i;
	char *p;
	static struct custom_salt *cs;

	cs = mem_calloc_tiny(sizeof(struct custom_salt), sizeof(uint64_t));

	ctcopy += TAG_LENGTH;
	p = strtokm(ctcopy, "*");
	p = strtokm(NULL, "*");
	cs->iterations = atoi(p);
	p = strtokm(NULL, "*");
	cs->salt_length = strlen(p);
	strncpy((char *)cs->salt, p, sizeof(cs->salt));
	cs->salt[sizeof(cs->salt) - 1] = 0;
	p = strtokm(NULL, "*");
	for (i = 0; i < IVLEN; i++)
		cs->iv[i] = atoi16[ARCH_INDEX(p[i * 2])] * 16
			+ atoi16[ARCH_INDEX(p[i * 2 + 1])];
	p = strtokm(NULL, "*");
	for (i = 0; i < BLOBLEN; i++)
		cs->blob[i] = atoi16[ARCH_INDEX(p[i * 2])] * 16
			+ atoi16[ARCH_INDEX(p[i * 2 + 1])];
	MEM_FREE(keeptr);

	return (void *)cs;
}

unsigned int bitwarden_common_iteration_count(void *salt)
{
	struct custom_salt *cs = salt;

	return (unsigned int) cs->iterations;
}