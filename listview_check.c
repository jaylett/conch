#include "checkrunner.h"

#include "blastlist.h"
#include "listview.h"

#define ASSERT_PTR_NULL(ptr) ck_assert_ptr_eq(ptr, NULL)
#define ASSERT_PTR_NOT_NULL(ptr) ck_assert_ptr_ne(ptr, NULL)

START_TEST(test_listview_new) {
  screen_state_s *lv = conch_listview_new(false);

  ASSERT_PTR_NOT_NULL(lv);
  ASSERT_PTR_NULL(lv->head);
  ASSERT_PTR_NULL(lv->current_blast);
  ck_assert_int_eq(lv->blast_offset, 0);
  ck_assert_int_eq(lv->stick_to_top, false);

  conch_listview_free(lv);
}
END_TEST

START_TEST(test_listview_new_with_stick_to_top) {
  screen_state_s *lv = conch_listview_new(true);

  ASSERT_PTR_NOT_NULL(lv);
  ck_assert_int_eq(lv->stick_to_top, true);

  conch_listview_free(lv);
}
END_TEST

START_TEST(test_listview_update_null_blastlist) {
  blastlist *bl = NULL;
  screen_state_s *lv = conch_listview_new(1);

  conch_listview_update(lv, bl);

  ASSERT_PTR_NULL(lv->head);
  ASSERT_PTR_NULL(lv->current_blast);

  conch_listview_free(lv);
}
END_TEST

START_TEST(test_listview_update_sets_current_if_null) {
  blastlist *bl = conch_blastlist_new();
  screen_state_s *lv = conch_listview_new(true);

  conch_listview_update(lv, bl);

  // On first update, both head and current_blast should be set to the head of
  // the passed blastlist.
  ck_assert_ptr_eq(lv->head, bl);
  ck_assert_ptr_eq(lv->current_blast, bl);

  conch_listview_free(lv);
}
END_TEST

START_TEST(test_listview_update_does_not_set_current_otherwise) {
  blastlist *bl1 = conch_blastlist_new();
  blastlist *bl2 = conch_blastlist_join(conch_blastlist_new(), bl1);
  screen_state_s *lv = conch_listview_new(true);

  // If update is called and current_blast is already set, it should not be
  // overridden.
  conch_listview_update(lv, bl1);
  conch_listview_update(lv, bl2);

  ck_assert_ptr_eq(lv->head, bl2);
  ck_assert_ptr_eq(lv->current_blast, bl1);

  conch_listview_free(lv);
}
END_TEST

Suite *listview_suite(void) {
  Suite *s = suite_create("listview");

  add_test_case(s, "create", test_listview_new);
  add_test_case(s, "create_with_stick_to_top",
                test_listview_new_with_stick_to_top);
  add_test_case(s, "update", test_listview_update_null_blastlist);
  add_test_case(s, "update_current", test_listview_update_sets_current_if_null);
  add_test_case(s, "update_current_notnull",
                test_listview_update_does_not_set_current_otherwise);

  return s;
}

CONCH_CHECK_MAIN(listview_suite)
