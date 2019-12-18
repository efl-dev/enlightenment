#include "e.h"
#include <time.h>
#include <sys/mman.h>

#if defined(__FreeBSD__) || defined(__DragonFly__)
# include <sys/types.h>
# include <sys/sysctl.h>
#endif

extern E_Module *shot_module;

#define MAXZONES 64

void         share_save              (const char *cmd);
void         share_write_end_watch   (void *data);
void         share_write_status_watch(void *data);
void         share_dialog_show       (void);
void         share_confirm           (void);
Eina_Bool    share_have              (void);
void         share_abort             (void);
void         preview_dialog_show     (E_Zone *zone, E_Client *ec, const char *params, void *dst, int sw, int sh);
Eina_Bool    preview_have            (void);
void         preview_abort           (void);
Evas_Object *preview_image_get       (void);
void         save_to                 (const char *file);
void         save_dialog_show        (void);
Eina_Bool    save_have               (void);
void         save_abort              (void);
