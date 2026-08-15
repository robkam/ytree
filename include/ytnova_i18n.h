#ifndef YTNOVA_I18N_H
#define YTNOVA_I18N_H

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *I18n_Gettext(const char *msgid);
const char *I18n_PGettext(const char *context, const char *msgid);
int I18n_VFormat(char *dest, size_t dest_size, const char *msgid, va_list ap);
int I18n_Format(char *dest, size_t dest_size, const char *msgid, ...);
void I18n_Init(void);
const char *I18n_GetLanguage(void);

#define _(msgid) I18n_Gettext(msgid)
#define N_(msgid) (msgid)
#define P_(context, msgid) I18n_PGettext((context), (msgid))
#define NP_(context, msgid) (msgid)

#ifdef __cplusplus
}
#endif

#endif
