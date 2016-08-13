#include <Elementary.h>

#define TITLE  "Enter Your Password"
#define TEXT   "Please enter your user password"
#define GUIDE  "Password"
#define OK     "OK"
#define CANCEL "Cancel"
#define PAD    "pad_medium"

int ret = -1;

static Evas_Object *entry = NULL;

static void
password_out(void)
{
   const char *str = elm_object_text_get(entry);
   if (str) printf("%s\n", str);
}

static void
cb_ok(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   password_out();
   ret = 0;
   elm_exit();
}

static void
cb_cancel(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   elm_exit();
}

EAPI int
elm_main(int argc, char **argv)
{
   Evas_Object *win, *bx, *fr, *lab, *en, *sep, *bx2, *bt;

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

   elm_app_compile_bin_dir_set(PACKAGE_BIN_DIR);
   elm_app_compile_lib_dir_set(PACKAGE_LIB_DIR);
   elm_app_compile_data_dir_set(PACKAGE_DATA_DIR);
   elm_app_info_set(elm_main, "enlightenment", "AUTHORS");

   do {
      win = elm_win_util_standard_add("main", TITLE);
      elm_win_autodel_set(win, EINA_TRUE);
      do {
         bx = elm_box_add(win);
         elm_box_horizontal_set(bx, EINA_FALSE);
         evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
         elm_win_resize_object_add(win, bx);
         do {
            fr = elm_frame_add(win);
            evas_object_size_hint_align_set(fr, EVAS_HINT_FILL, EVAS_HINT_FILL);
            elm_object_style_set(fr, PAD);
            elm_box_pack_end(bx, fr);
            do {
               lab = elm_label_add(win);
               evas_object_size_hint_align_set(lab, EVAS_HINT_FILL, 0.5);
               elm_object_text_set(lab, TEXT);
               elm_object_content_set(fr, lab);
               evas_object_show(lab);
            } while (0);
            evas_object_show(fr);
         } while (0);
         do {
            fr = elm_frame_add(win);
            evas_object_size_hint_align_set(fr, EVAS_HINT_FILL, EVAS_HINT_FILL);
            elm_object_style_set(fr, PAD);
            elm_box_pack_end(bx, fr);
            do {
               en = elm_entry_add(win);
               entry = en;
               elm_entry_scrollable_set(en, EINA_TRUE);
               evas_object_size_hint_weight_set(en, EVAS_HINT_EXPAND, 0.0);
               evas_object_size_hint_align_set(en, EVAS_HINT_FILL, 0.5);
               elm_scroller_policy_set(en, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
               elm_object_part_text_set(en, "guide", GUIDE);
               elm_entry_single_line_set(en, EINA_TRUE);
               elm_entry_password_set(en, EINA_TRUE);
               evas_object_smart_callback_add(en, "activated", cb_ok, NULL);
               evas_object_smart_callback_add(en, "aborted", cb_cancel, NULL);
               elm_object_content_set(fr, en);
               evas_object_show(en);
               elm_object_focus_set(en, EINA_TRUE);
            } while (0);
            evas_object_show(fr);
         } while (0);
         do {
            sep = elm_separator_add(win);
            elm_separator_horizontal_set(sep, EINA_TRUE);
            evas_object_size_hint_weight_set(sep, EVAS_HINT_EXPAND, 0.0);
            evas_object_size_hint_align_set(sep, EVAS_HINT_FILL, EVAS_HINT_FILL);
            elm_box_pack_end(bx, sep);
            evas_object_show(sep);
         } while (0);
         do {
            fr = elm_frame_add(win);
            evas_object_size_hint_align_set(fr, EVAS_HINT_FILL, EVAS_HINT_FILL);
            elm_object_style_set(fr, PAD);
            elm_box_pack_end(bx, fr);
            do {
               bx2 = elm_box_add(win);
               elm_box_horizontal_set(bx2, EINA_TRUE);
               elm_box_homogeneous_set(bx2, EINA_TRUE);
               evas_object_size_hint_align_set(bx2, 0.5, 0.5);
               elm_object_content_set(fr, bx2);
               do {
                  bt = elm_button_add(win);
                  elm_object_text_set(bt, OK);
                  evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
                  evas_object_smart_callback_add(bt, "clicked", cb_ok, NULL);
                  elm_box_pack_end(bx2, bt);
                  evas_object_show(bt);
               } while (0);
               do {
                  bt = elm_button_add(win);
                  elm_object_text_set(bt, CANCEL);
                  evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
                  evas_object_smart_callback_add(bt, "clicked", cb_cancel, NULL);
                  elm_box_pack_end(bx2, bt);
                  evas_object_show(bt);
               } while (0);
               evas_object_show(bx2);
            } while (0);
            evas_object_show(fr);
         } while (0);
         evas_object_show(bx);
      } while (0);
      elm_win_center(win, EINA_TRUE, EINA_TRUE);
      evas_object_show(win);
   } while (0);

   elm_win_activate(win);

   elm_run();
   return ret;
}
ELM_MAIN()