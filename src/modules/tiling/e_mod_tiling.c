#include "e_mod_tiling.h"

/* types {{{ */

#define TILING_OVERLAY_TIMEOUT 5.0
#define TILING_RESIZE_STEP 5
#define TILING_WRAP_SPEED 0.1

typedef struct geom_t {
    int x, y, w, h;
} geom_t;

typedef struct Client_Extra {
    E_Client *client;
    geom_t expected;
    struct {
         geom_t geom;
         E_Maximize  maximized;
         const char *bordername;
    } orig;
    int last_frame_adjustment; // FIXME: Hack for frame resize bug.
    Eina_Bool sticky : 1;
    Eina_Bool floating : 1;
    Eina_Bool tiled : 1;
} Client_Extra;

struct tiling_g tiling_g = {
    .module = NULL,
    .config = NULL,
    .log_domain = -1,
};

static void _add_client(E_Client *ec);
static void _remove_client(E_Client *ec);
static void _foreach_desk(void (*func)(E_Desk *desk));

/* }}} */
/* Globals {{{ */

static struct tiling_mod_main_g
{
    char                  edj_path[PATH_MAX];
    E_Config_DD          *config_edd,
                         *vdesk_edd;
    int                   currently_switching_desktop;
    Ecore_X_Window        action_input_win;
    Ecore_Event_Handler  *handler_key,
                         *handler_client_resize,
                         *handler_client_move,
                         *handler_client_add,
                         *handler_client_remove,
                         *handler_client_iconify,
                         *handler_client_uniconify,
                         *handler_client_stick,
                         *handler_client_unstick,
                         *handler_desk_show,
                         *handler_desk_before_show,
                         *handler_desk_set,
                         *handler_compositor_resize;
    E_Client_Hook        *pre_client_assign_hook;
    E_Client_Menu_Hook   *client_menu_hook;

    Tiling_Info          *tinfo;
    Eina_Hash            *info_hash;
    Eina_Hash            *client_extras;
    Eina_Hash            *overlays;

    E_Action             *act_togglefloat,
                         *act_move_up,
                         *act_move_down,
                         *act_move_left,
                         *act_move_right,
                         *act_toggle_split_mode;

    int                   warp_x,
                          warp_y,
                          old_warp_x,
                          old_warp_y,
                          warp_to_x,
                          warp_to_y;
    Ecore_Timer          *warp_timer;

    Ecore_Timer          *action_timer;
    E_Client             *focused_ec;
    void (*action_cb)(E_Client *ec, Client_Extra *extra);
    Tiling_Split_Type     split_type;

    char                  keys[4];
} _G = {
    .split_type = TILING_SPLIT_HORIZONTAL,
};

/* }}} */
/* Utils {{{ */

/* I wonder why noone has implemented the following one yet? */
static E_Desk *
get_current_desk(void)
{
    E_Manager *m = e_manager_current_get();
    E_Comp *c = m->comp;
    E_Zone *z = e_zone_current_get(c);

    return e_desk_current_get(z);
}

static Tiling_Info *
_initialize_tinfo(const E_Desk *desk)
{
    Tiling_Info *tinfo;

    tinfo = E_NEW(Tiling_Info, 1);
    tinfo->desk = desk;
    eina_hash_direct_add(_G.info_hash, &tinfo->desk, tinfo);

    tinfo->conf = get_vdesk(tiling_g.config->vdesks, desk->x, desk->y,
                            desk->zone->num);

    return tinfo;
}

static void
check_tinfo(const E_Desk *desk)
{
    if (!_G.tinfo || _G.tinfo->desk != desk) {
        _G.tinfo = eina_hash_find(_G.info_hash, &desk);
        if (!_G.tinfo) {
            /* lazy init */
            _G.tinfo = _initialize_tinfo(desk);
        }
        if (!_G.tinfo->conf) {
            _G.tinfo->conf = get_vdesk(tiling_g.config->vdesks,
                                       desk->x, desk->y,
                                       desk->zone->num);
        }
    }
}

static Eina_Bool
desk_should_tile_check(const E_Desk *desk)
{
   check_tinfo(desk);
   return (_G.tinfo && _G.tinfo->conf &&_G.tinfo->conf->nb_stacks);
}

static int
is_ignored_window(const Client_Extra *extra)
{
   if (extra->sticky || extra->floating)
      return true;

   return false;
}

static int
is_tilable(const E_Client *ec)
{
    if (ec->icccm.min_h == ec->icccm.max_h
    &&  ec->icccm.max_h > 0)
        return false;

    if (ec->icccm.gravity == ECORE_X_GRAVITY_STATIC)
        return false;

    if (ec->e.state.centered)
       return false;

    if (!tiling_g.config->tile_dialogs
            && ((ec->icccm.transient_for != 0)
                || (ec->netwm.type == E_WINDOW_TYPE_DIALOG)))
        return false;

    if (ec->fullscreen) {
         return false;
    }

    return true;
}

static void
change_window_border(E_Client   *ec,
                     const char *bordername)
{
    eina_stringshare_replace(&ec->bordername, bordername);
    ec->border.changed = true;
    ec->changes.border = true;
    ec->changed = true;

    DBG("%p -> border %s", ec, bordername);
}

static Eina_Bool
_info_hash_update(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__,
                  void *data, void *fdata __UNUSED__)
{
    Tiling_Info *tinfo = data;

    if (tinfo->desk) {
        tinfo->conf = get_vdesk(tiling_g.config->vdesks,
                                tinfo->desk->x, tinfo->desk->y,
                                tinfo->desk->zone->num);
    } else {
        tinfo->conf = NULL;
    }

    return true;
}

void
e_tiling_update_conf(void)
{
    eina_hash_foreach(_G.info_hash, _info_hash_update, NULL);
}

static void
_e_client_move_resize(E_Client *ec,
                      int       x,
                      int       y,
                      int       w,
                      int       h)
{
    Client_Extra *extra;

    extra = eina_hash_find(_G.client_extras, &ec);
    if (!extra) {
         ERR("No extra for %p", ec);
         return;
    }

    extra->last_frame_adjustment = MAX(ec->h - ec->client.h, ec->w - ec->client.w);
    DBG("%p -> %dx%d+%d+%d", ec, w, h, x, y);
    evas_object_geometry_set(ec->frame, x, y, w, h);
}

static void
_e_client_unmaximize(E_Client *ec, E_Maximize max)
{
    DBG("%p -> %s", ec,
        (max & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_NONE ? "NONE" :
        (max & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_VERTICAL ? "VERTICAL" :
        (max & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_HORIZONTAL ? "HORIZONTAL" :
        "BOTH");
    e_client_unmaximize(ec, max);
}

static void
_restore_client(E_Client *ec)
{
    Client_Extra *extra;

    extra = eina_hash_find(_G.client_extras, &ec);
    if (!extra) {
         ERR("No extra for %p", ec);
         return;
    }
    _e_client_move_resize(ec,
                          extra->orig.geom.x,
                          extra->orig.geom.y,
                          extra->orig.geom.w,
                          extra->orig.geom.h);
    if (extra->orig.maximized) {
        e_client_maximize(ec, extra->orig.maximized);
        ec->maximized = extra->orig.maximized;
    }

    DBG("Change window border back to %s for %p",
        extra->orig.bordername, ec);
    change_window_border(ec, (extra->orig.bordername)
                         ? extra->orig.bordername : "default");
}

static Client_Extra *
_get_or_create_client_extra(E_Client *ec)
{
    Client_Extra *extra;

    extra = eina_hash_find(_G.client_extras, &ec);
    if (!extra) {
        extra = E_NEW(Client_Extra, 1);
        *extra = (Client_Extra) {
            .client = ec,
            .expected = {
                .x = ec->x,
                .y = ec->y,
                .w = ec->w,
                .h = ec->h,
            },
            .orig = {
                .geom = {
                    .x = ec->x,
                    .y = ec->y,
                    .w = ec->w,
                    .h = ec->h,
                },
                .maximized = ec->maximized,
                .bordername = eina_stringshare_add(ec->bordername),
            },
        };
        eina_hash_direct_add(_G.client_extras, &extra->client, extra);
    } else {
        extra->expected = (geom_t) {
            .x = ec->x,
            .y = ec->y,
            .w = ec->w,
            .h = ec->h,
        };
        extra->orig.geom = extra->expected;
        extra->orig.maximized = ec->maximized;
        eina_stringshare_replace(&extra->orig.bordername, ec->bordername);
    }

    return extra;
}

void
tiling_e_client_move_resize_extra(E_Client *ec,
                      int       x,
                      int       y,
                      int       w,
                      int       h)
{
    Client_Extra *extra = eina_hash_find(_G.client_extras, &ec);
    if (!extra) {
        ERR("No extra for %p", ec);
        return;
    }

    extra->expected = (geom_t) {
         .x = x,
         .y = y,
         .w = w,
         .h = h,
    };

   _e_client_move_resize(ec, x, y, w, h);
}

/* }}} */
/* Reorganize Stacks {{{*/

static void
_reapply_tree(void)
{
     int zx, zy, zw, zh;

     if (_G.tinfo->tree)
       {
          e_zone_useful_geometry_get(_G.tinfo->desk->zone, &zx, &zy, &zw, &zh);

          tiling_window_tree_apply(_G.tinfo->tree, zx, zy, zw, zh);
       }
}

void
_restore_free_client(void *_item)
{
   Window_Tree *item = _item;
   if (item->client)
     {
        _restore_client(item->client);
     }
   free(item);
}

void
change_desk_conf(struct _Config_vdesk *newconf)
{
    E_Manager *m;
    E_Comp *c;
    E_Zone *z;
    E_Desk *d;
    int old_nb_stacks = 0,
        new_nb_stacks = newconf->nb_stacks;

    m = e_manager_current_get();
    if (!m) return;
    c = m->comp;
    z = e_comp_zone_number_get(c, newconf->zone_num);
    if (!z) return;
    d = e_desk_at_xy_get(z, newconf->x, newconf->y);
    if (!d) return;

    check_tinfo(d);
    if (_G.tinfo->conf) {
        old_nb_stacks = _G.tinfo->conf->nb_stacks;
    } else {
        newconf->nb_stacks = 0;
    }
    _G.tinfo->conf = newconf;
    _G.tinfo->conf->nb_stacks = old_nb_stacks;

    if (new_nb_stacks == old_nb_stacks)
        return;

    if (new_nb_stacks == 0) {
        tiling_window_tree_walk(_G.tinfo->tree, _restore_free_client);
        _G.tinfo->tree = NULL;
        e_place_zone_region_smart_cleanup(z);
    }
    _G.tinfo->conf->nb_stacks = new_nb_stacks;
}

/* }}} */
/* Reorganize windows {{{*/

static void
_add_client(E_Client *ec)
{
    /* Should I need to check that the client is not already added? */
    if (!ec) {
        return;
    }
    if (!is_tilable(ec)) {
        return;
    }

    if (!desk_should_tile_check(ec->desk))
        return;

    Client_Extra *extra = _get_or_create_client_extra(ec);

    if (is_ignored_window(extra))
       return;

    if (extra->tiled)
       return;

    extra->tiled = EINA_TRUE;

    DBG("adding %p", ec);

    if (ec->maximized)
       _e_client_unmaximize(ec, E_MAXIMIZE_BOTH);

    /* Window tree updating. */
      {
         E_Client *ec_focused = e_client_focused_get();
         /* If focused is NULL, it should return the root. */
         Window_Tree *parent = tiling_window_tree_client_find(_G.tinfo->tree,
               ec_focused);
         if (!parent && (ec_focused != ec))
           {
              Client_Extra *extra_focused = eina_hash_find(_G.client_extras, &ec_focused);
              if (_G.tinfo->tree && extra_focused && !is_ignored_window(extra_focused))
                {
                   ERR("Couldn't find tree item for focused client %p. Using root..",
                         e_client_focused_get());
                }
           }

         _G.tinfo->tree = tiling_window_tree_add(_G.tinfo->tree, parent, ec, _G.split_type);
      }

    _reapply_tree();
}

static void
_remove_client(E_Client *ec)
{
    if (!ec)
       return;

    DBG("removing %p", ec);

    Client_Extra *extra = eina_hash_find(_G.client_extras, &ec);
    if (!extra)
      {
         if (is_tilable(ec))
           {
              ERR("No extra for %p", ec);
           }
         return;
      }

    extra->tiled = EINA_FALSE;

    /* Window tree updating. */
      {
         /* If focused is NULL, it should return the root. */
         Window_Tree *item = tiling_window_tree_client_find(_G.tinfo->tree, ec);
         if (!item)
           {
              ERR("Couldn't find tree item for client %p!", ec);
              return;
           }

         _G.tinfo->tree = tiling_window_tree_remove(_G.tinfo->tree, item);
      }

    eina_hash_del(_G.client_extras, ec, NULL);

    _reapply_tree();
}

/* }}} */
/* Toggle Floating {{{ */

static void
toggle_floating(E_Client *ec)
{
    if (!ec)
        return;
    if (!desk_should_tile_check(ec->desk))
        return;

    Client_Extra *extra = eina_hash_find(_G.client_extras, &ec);
    if (!extra) {
        ERR("No extra for %p", ec);
        return;
    }

    extra->floating = !extra->floating;

    /* This is the new state, act accordingly. */
    if (extra->floating)
      {
        _remove_client(ec);
        _restore_client(ec);
      }
    else
      {
        _add_client(ec);
      }
}

static void
_e_mod_action_toggle_floating_cb(E_Object   *obj __UNUSED__,
                                 const char *params __UNUSED__)
{
    toggle_floating(e_client_focused_get());
}

static void
_e_mod_menu_border_cb(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   E_Client *ec = data;

   toggle_floating(ec);
}

/* }}} */
/* {{{ Move windows */

static void
_action_swap(int cross_edge)
{
    E_Desk *desk;
    E_Client *focused_ec;

    desk = get_current_desk();
    if (!desk)
        return;

    focused_ec = e_client_focused_get();
    if (!focused_ec || focused_ec->desk != desk)
        return;

    if (!desk_should_tile_check(desk))
        return;

    Window_Tree *item = tiling_window_tree_client_find(_G.tinfo->tree, focused_ec);

    if (item)
      {
         tiling_window_tree_node_move(item, cross_edge);

         _reapply_tree();
      }
}

static void
_e_mod_action_move_left_cb(E_Object   *obj __UNUSED__,
                      const char *params __UNUSED__)
{
   _action_swap(TILING_WINDOW_TREE_EDGE_LEFT);
}

static void
_e_mod_action_move_right_cb(E_Object   *obj __UNUSED__,
                      const char *params __UNUSED__)
{
   _action_swap(TILING_WINDOW_TREE_EDGE_RIGHT);
}

static void
_e_mod_action_move_up_cb(E_Object   *obj __UNUSED__,
                      const char *params __UNUSED__)
{
   _action_swap(TILING_WINDOW_TREE_EDGE_TOP);
}

static void
_e_mod_action_move_down_cb(E_Object   *obj __UNUSED__,
                      const char *params __UNUSED__)
{
   _action_swap(TILING_WINDOW_TREE_EDGE_BOTTOM);
}

/* }}} */
/* Toggle split mode {{{ */

static void
_e_mod_action_toggle_split_mode(E_Object   *obj __UNUSED__,
                                 const char *params __UNUSED__)
{
    E_Desk *desk;

    desk = get_current_desk();
    if (!desk)
        return;

    if (!desk_should_tile_check(desk))
        return;

    _G.split_type = (_G.split_type == TILING_SPLIT_VERTICAL) ?
       TILING_SPLIT_HORIZONTAL : TILING_SPLIT_VERTICAL;
}

/* }}} */
/* Hooks {{{*/

static void
_pre_client_assign_hook(void *data __UNUSED__,
                        E_Client *ec)
{
    if (tiling_g.config->show_titles)
        return;

    if (!ec) {
        return;
    }

    if (!desk_should_tile_check(ec->desk))
        return;

    if (!is_tilable(ec)) {
        return;
    }

    /* Fill initial values if not already done */
    Client_Extra *extra = _get_or_create_client_extra(ec);

    if (is_ignored_window(extra))
       return;

    if ((ec->bordername && strcmp(ec->bordername, "pixel"))
          || !ec->bordername)
    {
        change_window_border(ec, "pixel");
    }
}

static void _move_or_resize(E_Client *ec)
{
    Client_Extra *extra;

    if (!ec) {
        return;
    }
    if (!is_tilable(ec)) {
        return;
    }

    if (!desk_should_tile_check(ec->desk))
        return;

    DBG("Resize: %p / '%s' / '%s', (%d,%d), changes(size=%d, position=%d, client=%d)"
        " g:%dx%d+%d+%d ecname:'%s' maximized:%s fs:%s",
        ec, ec->icccm.title, ec->netwm.name,
        ec->desk->x, ec->desk->y,
        ec->changes.size, ec->changes.pos, ec->changes.border,
        ec->w, ec->h, ec->x, ec->y, ec->bordername,
        (ec->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_NONE ? "NONE" :
        (ec->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_VERTICAL ? "VERTICAL" :
        (ec->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_HORIZONTAL ? "HORIZONTAL" :
        "BOTH", ec->fullscreen? "true": "false");

    extra = eina_hash_find(_G.client_extras, &ec);
    if (!extra) {
        ERR("No extra for %p", ec);
        return;
    }

    if (is_ignored_window(extra))
       return;

    if ((ec->x == extra->expected.x) && (ec->y == extra->expected.y) &&
          (ec->w == extra->expected.w) && (ec->h == extra->expected.h))
      {

         return;
      }

    if (!extra->last_frame_adjustment)
      {
         printf("This is probably because of the frame adjustment bug. Return\n");
         _reapply_tree();
         return;
      }

    Window_Tree *item = tiling_window_tree_client_find(_G.tinfo->tree, ec);
    if (!item)
      {
         ERR("Couldn't find tree item for resized client %p!", ec);
         return;
      }

      {
         int w_dir = 1, h_dir = 1;
         double w_diff = 1.0, h_diff = 1.0;
         if (abs(extra->expected.w - ec->w) >= 1)
           {
              w_diff = ((double) ec->w) / extra->expected.w;
           }
         if (abs(extra->expected.h - ec->h) >= 1)
           {
              h_diff = ((double) ec->h) / extra->expected.h;
           }
         switch (ec->resize_mode)
           {
            case E_POINTER_RESIZE_L:
            case E_POINTER_RESIZE_BL:
               w_dir = -1;
               break;
            case E_POINTER_RESIZE_T:
            case E_POINTER_RESIZE_TR:
               h_dir = -1;
               break;
            case E_POINTER_RESIZE_TL:
               w_dir = -1;
               h_dir = -1;
               break;
            default:
               break;
           }
         if ((w_diff != 1.0) || (h_diff != 1.0))
           {
              if (!tiling_window_tree_node_resize(item, w_dir, w_diff, h_dir, h_diff))
                {
                   /* FIXME: Do something? */
                }
           }
      }

    _reapply_tree();
}

static void
_resize_begin_hook(void *data EINA_UNUSED, E_Client *ec)
{
    Client_Extra *extra;

    if (!ec) {
        return;
    }
    if (!is_tilable(ec)) {
        return;
    }

    if (!desk_should_tile_check(ec->desk))
        return;

    extra = eina_hash_find(_G.client_extras, &ec);
    if (!extra) {
        ERR("No extra for %p", ec);
        return;
    }

    if (is_ignored_window(extra))
       return;

    Window_Tree *item = tiling_window_tree_client_find(_G.tinfo->tree, ec);
    if (!item)
      {
         ERR("Couldn't find tree item for resized client %p!", ec);
         return;
      }

    int edges = tiling_window_tree_edges_get(item);
    if (edges & TILING_WINDOW_TREE_EDGE_LEFT)
      {
         switch (ec->resize_mode)
           {
            case E_POINTER_RESIZE_L:
               ec->resize_mode = E_POINTER_RESIZE_NONE;
               break;
            case E_POINTER_RESIZE_TL:
               ec->resize_mode = E_POINTER_RESIZE_T;
               break;
            case E_POINTER_RESIZE_BL:
               ec->resize_mode = E_POINTER_RESIZE_B;
               break;
            default:
               break;
           }
      }
    if (edges & TILING_WINDOW_TREE_EDGE_RIGHT)
      {
         switch (ec->resize_mode)
           {
            case E_POINTER_RESIZE_R:
               ec->resize_mode = E_POINTER_RESIZE_NONE;
               break;
            case E_POINTER_RESIZE_TR:
               ec->resize_mode = E_POINTER_RESIZE_T;
               break;
            case E_POINTER_RESIZE_BR:
               ec->resize_mode = E_POINTER_RESIZE_B;
               break;
            default:
               break;
           }
      }
    if (edges & TILING_WINDOW_TREE_EDGE_TOP)
      {
         switch (ec->resize_mode)
           {
            case E_POINTER_RESIZE_T:
               ec->resize_mode = E_POINTER_RESIZE_NONE;
               break;
            case E_POINTER_RESIZE_TL:
               ec->resize_mode = E_POINTER_RESIZE_L;
               break;
            case E_POINTER_RESIZE_TR:
               ec->resize_mode = E_POINTER_RESIZE_R;
               break;
            default:
               break;
           }
      }
    if (edges & TILING_WINDOW_TREE_EDGE_BOTTOM)
      {
         switch (ec->resize_mode)
           {
            case E_POINTER_RESIZE_B:
               ec->resize_mode = E_POINTER_RESIZE_NONE;
               break;
            case E_POINTER_RESIZE_BL:
               ec->resize_mode = E_POINTER_RESIZE_L;
               break;
            case E_POINTER_RESIZE_BR:
               ec->resize_mode = E_POINTER_RESIZE_R;
               break;
            default:
               break;
           }
      }
}

static Eina_Bool
_resize_hook(void *data __UNUSED__, int type __UNUSED__, E_Event_Client *event)
{
    E_Client *ec = event->ec;

    _move_or_resize(ec);

    return true;
}

static Eina_Bool
_move_hook(void *data __UNUSED__, int type __UNUSED__, E_Event_Client*event)
{
    E_Client *ec = event->ec;
    if (!ec) {
        return true;
    }
    if (!is_tilable(ec)) {
        return true;
    }

    if (!desk_should_tile_check(ec->desk))
        return true;

    Client_Extra *extra = eina_hash_find(_G.client_extras, &ec);
    if (!extra) {
        ERR("No extra for %p", ec);
        return true;
    }

    if (is_ignored_window(extra))
       return true;

    e_client_act_move_end(event->ec, NULL);

    _reapply_tree();

    return true;
}

static Eina_Bool
_add_hook(void *data __UNUSED__, int type __UNUSED__, E_Event_Client *event)
{
    E_Client *ec = event->ec;

    if (e_client_util_ignored_get(ec)) return ECORE_CALLBACK_RENEW;

    DBG("Add: %p / '%s' / '%s', (%d,%d), changes(size=%d, position=%d, client=%d)"
        " g:%dx%d+%d+%d ecname:'%s' maximized:%s fs:%s",
        ec, ec->icccm.title, ec->netwm.name,
        ec->desk->x, ec->desk->y,
        ec->changes.size, ec->changes.pos, ec->changes.border,
        ec->w, ec->h, ec->x, ec->y, ec->bordername,
        (ec->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_NONE ? "NONE" :
        (ec->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_VERTICAL ? "VERTICAL" :
        (ec->maximized & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_HORIZONTAL ? "HORIZONTAL" :
        "BOTH", ec->fullscreen? "true": "false");

    _add_client(ec);

    return true;
}

static Eina_Bool
_remove_hook(void *data __UNUSED__, int type __UNUSED__, E_Event_Client *event)
{
    E_Client *ec = event->ec;

    if (e_client_util_ignored_get(ec)) return ECORE_CALLBACK_RENEW;

    if (_G.currently_switching_desktop)
        return true;

    if (!desk_should_tile_check(ec->desk))
        return true;

    _remove_client(ec);

    return true;
}

static bool
_iconify_hook(void *data __UNUSED__, int type __UNUSED__, E_Event_Client *event)
{
    E_Client *ec = event->ec;

    DBG("iconify hook: %p", ec);

    if (ec->deskshow)
        return true;

    if (!desk_should_tile_check(ec->desk))
        return true;

    Client_Extra *extra = eina_hash_find(_G.client_extras, &ec);
    if (!extra) {
        ERR("No extra for %p", ec);
        return true;
    }

    if (is_ignored_window(extra))
       return true;

    _remove_client(ec);

    return true;
}

static bool
_uniconify_hook(void *data __UNUSED__, int type __UNUSED__, E_Event_Client *event)
{
    E_Client *ec = event->ec;

    if (ec->deskshow)
        return true;

    if (!desk_should_tile_check(ec->desk))
        return true;

    if (!is_tilable(ec)) {
        return true;
    }

    _add_client(ec);

    return true;
}

static void
toggle_sticky(E_Client *ec)
{
    if (!ec)
        return;
    if (!desk_should_tile_check(ec->desk))
        return;

    Client_Extra *extra = eina_hash_find(_G.client_extras, &ec);
    if (!extra) {
        ERR("No extra for %p", ec);
        return;
    }

    extra->sticky = !extra->sticky;

    /* This is the new state, act accordingly. */
    if (extra->sticky)
      {
        _remove_client(ec);
        _restore_client(ec);
      }
    else
      {
        _add_client(ec);
      }
}

static Eina_Bool
_stick_hook(void *data __UNUSED__, int type __UNUSED__, E_Event_Client *event)
{
   toggle_sticky(event->ec);
   return true;
}

static Eina_Bool
_unstick_hook(void *data __UNUSED__, int type __UNUSED__, E_Event_Client *event)
{
   toggle_sticky(event->ec);
   return true;
}

static Eina_Bool
_desk_show_hook(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
    _G.currently_switching_desktop = 0;

    return true;
}

static Eina_Bool
_desk_before_show_hook(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
    _G.currently_switching_desktop = 1;

    return true;
}

static bool
_desk_set_hook(void *data __UNUSED__, int type __UNUSED__, E_Event_Client_Desk_Set *ev)
{
    DBG("%p: from (%d,%d) to (%d,%d)", ev->ec,
        ev->desk->x, ev->desk->y,
        ev->ec->desk->x, ev->ec->desk->y);

    if (desk_should_tile_check(ev->desk))
      {
         if (tiling_window_tree_client_find(_G.tinfo->tree, ev->ec)) {
              _remove_client(ev->ec);
              _restore_client(ev->ec);
         }
      }

    if (!desk_should_tile_check(ev->ec->desk))
        return true;

    _add_client(ev->ec);

    return true;
}

static void
_compositor_resize_hook_desk_reapply(E_Desk *desk)
{
   check_tinfo(desk);
   if (!desk_should_tile_check(desk))
      return;

   _reapply_tree();
}

static bool
_compositor_resize_hook(void *data __UNUSED__, int type __UNUSED__, E_Event_Compositor_Resize *ev EINA_UNUSED)
{
   _foreach_desk(_compositor_resize_hook_desk_reapply);

   return true;
}

static void
_bd_hook(void *d __UNUSED__, E_Client *ec)
{
   E_Menu_Item *mi;
   E_Menu *m;
   Eina_List *l;
   if (!ec->border_menu) return;
   m = ec->border_menu;

    Client_Extra *extra = eina_hash_find(_G.client_extras, &ec);
    if (!extra) {
        return;
    }

   /* position menu item just before the last separator */
   EINA_LIST_REVERSE_FOREACH(m->items, l, mi)
      if (mi->separator) break;
   if ((!mi) || (!mi->separator)) return;
   l = eina_list_prev(l);
   mi = eina_list_data_get(l);
   if (!mi) return;

   mi = e_menu_item_new_relative(m, mi);
   e_menu_item_label_set(mi, _("Floating"));
   e_menu_item_check_set(mi, true);
   e_menu_item_toggle_set(mi, (extra->floating) ? true : false);
   e_menu_item_callback_set(mi, _e_mod_menu_border_cb, ec);
}

/* }}} */
/* Module setup {{{*/

static void
_clear_info_hash(void *data)
{
    Tiling_Info *ti = data;

    tiling_window_tree_free(ti->tree);
    ti->tree = NULL;
    E_FREE(ti);
}

static void
_clear_border_extras(void *data)
{
    Client_Extra *extra = data;

    eina_stringshare_del(extra->orig.bordername);

    E_FREE(extra);
}

EAPI E_Module_Api e_modapi =
{
    E_MODULE_API_VERSION,
    "Tiling"
};

EAPI void *
e_modapi_init(E_Module *m)
{
    E_Desk *desk;
    Eina_List *l;

    tiling_g.module = m;

    if (tiling_g.log_domain < 0) {
        tiling_g.log_domain = eina_log_domain_register("tiling", NULL);
        if (tiling_g.log_domain < 0) {
            EINA_LOG_CRIT("could not register log domain 'tiling'");
        }
    }

    _G.info_hash = eina_hash_pointer_new(_clear_info_hash);
    _G.client_extras = eina_hash_pointer_new(_clear_border_extras);

    _G.pre_client_assign_hook = e_client_hook_add(
        E_CLIENT_HOOK_EVAL_PRE_FRAME_ASSIGN, _pre_client_assign_hook, NULL);

#define HANDLER(_h, _e, _f)                                   \
    _h = ecore_event_handler_add(E_EVENT_##_e,                \
                                 (Ecore_Event_Handler_Cb) _f, \
                                 NULL);

    e_client_hook_add(E_CLIENT_HOOK_RESIZE_BEGIN, _resize_begin_hook, NULL);
    HANDLER(_G.handler_client_resize, CLIENT_RESIZE, _resize_hook);
    HANDLER(_G.handler_client_move, CLIENT_MOVE, _move_hook);
    HANDLER(_G.handler_client_add, CLIENT_ADD, _add_hook);
    HANDLER(_G.handler_client_remove, CLIENT_REMOVE, _remove_hook);

    HANDLER(_G.handler_client_iconify, CLIENT_ICONIFY, _iconify_hook);
    HANDLER(_G.handler_client_uniconify, CLIENT_UNICONIFY, _uniconify_hook);
    HANDLER(_G.handler_client_stick, CLIENT_STICK, _stick_hook);
    HANDLER(_G.handler_client_unstick, CLIENT_UNSTICK, _unstick_hook);

    HANDLER(_G.handler_desk_show, DESK_SHOW, _desk_show_hook);
    HANDLER(_G.handler_desk_before_show, DESK_BEFORE_SHOW, _desk_before_show_hook);
    HANDLER(_G.handler_desk_set, CLIENT_DESK_SET, _desk_set_hook);
    HANDLER(_G.handler_compositor_resize, COMPOSITOR_RESIZE, _compositor_resize_hook);
#undef HANDLER

#define ACTION_ADD(_act, _cb, _title, _value, _params, _example, _editable)  \
    {                                                                        \
        E_Action *_action = _act;                                            \
        const char *_name = _value;                                          \
        if ((_action = e_action_add(_name))) {                               \
            _action->func.go = _cb;                                          \
            e_action_predef_name_set(N_("Tiling"), _title, _name,            \
                                     _params, _example, _editable);          \
        }                                                                    \
    }

    /* Module's actions */
    ACTION_ADD(_G.act_togglefloat, _e_mod_action_toggle_floating_cb,
               N_("Toggle floating"), "toggle_floating",
               NULL, NULL, 0);

    ACTION_ADD(_G.act_move_up, _e_mod_action_move_up_cb,
               N_("Move the focused window up"), "move_up",
               NULL, NULL, 0);
    ACTION_ADD(_G.act_move_down, _e_mod_action_move_down_cb,
               N_("Move the focused window down"), "move_down",
               NULL, NULL, 0);
    ACTION_ADD(_G.act_move_left, _e_mod_action_move_left_cb,
               N_("Move the focused window left"), "move_left",
               NULL, NULL, 0);
    ACTION_ADD(_G.act_move_right, _e_mod_action_move_right_cb,
               N_("Move the focused window right"), "move_right",
               NULL, NULL, 0);

    ACTION_ADD(_G.act_toggle_split_mode, _e_mod_action_toggle_split_mode,
               N_("Toggle split mode"), "toggle_split_mode",
               NULL, NULL, 0);

#undef ACTION_ADD

    /* Configuration entries */
    snprintf(_G.edj_path, sizeof(_G.edj_path),"%s/e-module-tiling.edj",
             e_module_dir_get(m));
    e_configure_registry_category_add("windows", 50, _("Windows"), NULL,
                                      "preferences-system-windows");
    e_configure_registry_item_add("windows/tiling", 150, _("Tiling"),
                                  NULL, _G.edj_path,
                                  e_int_config_tiling_module);

    /* Configuration itself */
    _G.config_edd = E_CONFIG_DD_NEW("Tiling_Config", Config);
    _G.vdesk_edd = E_CONFIG_DD_NEW("Tiling_Config_VDesk",
                                   struct _Config_vdesk);
    E_CONFIG_VAL(_G.config_edd, Config, tile_dialogs, INT);
    E_CONFIG_VAL(_G.config_edd, Config, show_titles, INT);

    E_CONFIG_LIST(_G.config_edd, Config, vdesks, _G.vdesk_edd);
    E_CONFIG_VAL(_G.vdesk_edd, struct _Config_vdesk, x, INT);
    E_CONFIG_VAL(_G.vdesk_edd, struct _Config_vdesk, y, INT);
    E_CONFIG_VAL(_G.vdesk_edd, struct _Config_vdesk, zone_num, INT);
    E_CONFIG_VAL(_G.vdesk_edd, struct _Config_vdesk, nb_stacks, INT);

    tiling_g.config = e_config_domain_load("module.tiling", _G.config_edd);
    if (!tiling_g.config) {
        tiling_g.config = E_NEW(Config, 1);
        tiling_g.config->tile_dialogs = 1;
        tiling_g.config->show_titles = 1;
    }

    E_CONFIG_LIMIT(tiling_g.config->tile_dialogs, 0, 1);
    E_CONFIG_LIMIT(tiling_g.config->show_titles, 0, 1);

    for (l = tiling_g.config->vdesks; l; l = l->next) {
        struct _Config_vdesk *vd;

        vd = l->data;

        E_CONFIG_LIMIT(vd->nb_stacks, 0, TILING_MAX_STACKS);
    }

    _G.client_menu_hook = e_int_client_menu_hook_add(_bd_hook, NULL);

    desk = get_current_desk();
    _G.tinfo = _initialize_tinfo(desk);

    _G.currently_switching_desktop = 0;
    _G.action_cb = NULL;

    /* Add all the existing windows. */
      {
         E_Client *ec;
         E_CLIENT_FOREACH(e_comp_get(NULL), ec)
           {
              if (e_client_util_ignored_get(ec))
                 continue;

              _add_client(ec);
           }
      }

    return m;
}

static void
_disable_desk(E_Desk *desk)
{
   check_tinfo(desk);
   if (!_G.tinfo->conf)
      return;

   tiling_window_tree_walk(_G.tinfo->tree, _restore_free_client);
   _G.tinfo->tree = NULL;
}

static void
_disable_all_tiling(void)
{
    const Eina_List *l, *ll;
    E_Comp *comp;
    E_Zone *zone;

    _foreach_desk(_disable_desk);

    EINA_LIST_FOREACH(e_comp_list(), l, comp) {
        EINA_LIST_FOREACH(comp->zones, ll, zone) {
            e_place_zone_region_smart_cleanup(zone);
        }
    }
}

static void
_foreach_desk(void (*func)(E_Desk *desk))
{
    const Eina_List *l, *ll;
    E_Comp *comp;
    E_Zone *zone;
    E_Desk *desk;
    int x, y;

    EINA_LIST_FOREACH(e_comp_list(), l, comp) {
        EINA_LIST_FOREACH(comp->zones, ll, zone) {
            for (x = 0; x < zone->desk_x_count; x++) {
                for (y = 0; y < zone->desk_y_count; y++) {
                    desk = zone->desks[x + (y * zone->desk_x_count)];

                    func(desk);
                }
            }
        }
    }
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
    _disable_all_tiling();

    e_int_client_menu_hook_del(_G.client_menu_hook);

    if (tiling_g.log_domain >= 0) {
        eina_log_domain_unregister(tiling_g.log_domain);
        tiling_g.log_domain = -1;
    }

    if (_G.pre_client_assign_hook) {
        e_client_hook_del(_G.pre_client_assign_hook);
        _G.pre_client_assign_hook = NULL;
    }

#define FREE_HANDLER(x)              \
    if (x) {                         \
        ecore_event_handler_del(x);  \
        x = NULL;                    \
    }
    FREE_HANDLER(_G.handler_client_resize);
    FREE_HANDLER(_G.handler_client_move);
    FREE_HANDLER(_G.handler_client_add);
    FREE_HANDLER(_G.handler_client_remove);

    FREE_HANDLER(_G.handler_client_iconify);
    FREE_HANDLER(_G.handler_client_uniconify);
    FREE_HANDLER(_G.handler_client_stick);
    FREE_HANDLER(_G.handler_client_unstick);

    FREE_HANDLER(_G.handler_desk_show);
    FREE_HANDLER(_G.handler_desk_before_show);
    FREE_HANDLER(_G.handler_desk_set);
#undef FREE_HANDLER


#define ACTION_DEL(act, title, value)                        \
    if (act) {                                               \
        e_action_predef_name_del("Tiling", title);           \
        e_action_del(value);                                 \
        act = NULL;                                          \
    }
    ACTION_DEL(_G.act_togglefloat, "Toggle floating", "toggle_floating");
    ACTION_DEL(_G.act_move_up, "Move the focused window up", "move_up");
    ACTION_DEL(_G.act_move_down, "Move the focused window down", "move_down");
    ACTION_DEL(_G.act_move_left, "Move the focused window left", "move_left");
    ACTION_DEL(_G.act_move_right, "Move the focused window right", "move_right");

    ACTION_DEL(_G.act_toggle_split_mode, "Toggle split mode",
          "toggle_split_mode");
#undef ACTION_DEL

    e_configure_registry_item_del("windows/tiling");
    e_configure_registry_category_del("windows");

    E_FREE(tiling_g.config);
    E_CONFIG_DD_FREE(_G.config_edd);
    E_CONFIG_DD_FREE(_G.vdesk_edd);

    tiling_g.module = NULL;

    eina_hash_free(_G.info_hash);
    _G.info_hash = NULL;

    eina_hash_free(_G.client_extras);
    _G.client_extras = NULL;

    _G.tinfo = NULL;

    return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
    e_config_domain_save("module.tiling", _G.config_edd, tiling_g.config);

    return true;
}
/* }}} */
