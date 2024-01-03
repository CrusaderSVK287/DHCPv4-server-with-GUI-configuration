#include "greatest.h"
#include "tests.h"
#include "../src/utils/llist.h"

TEST test_llist_append() {
	llist_t *list = llist_new();

	int data1 = 10;
	int data2 = 20;

	llist_append(list, &data1, false);
	llist_append(list, &data2, false);

	ASSERT_EQ(list->first->data, &data1);
	ASSERT_EQ(list->last->data, &data2);

        llist_destroy(&list);

	PASS();
}


TEST test_llist_get_index() {
	llist_t *list = llist_new();

        int data[] = {10, 20, 30, 40};
        for (int i = 0; i < sizeof(data) / sizeof(data[0]); ++i) {
                llist_append(list, &data[i], false);
        }

        llnode_t *node_at_2 = llist_get_index(list, 2);
        llnode_t *node_at_5 = llist_get_index(list, 5);

        ASSERT_EQ(node_at_2->data, &data[2]);
        ASSERT_EQ(node_at_5, NULL); // Index out of bounds

        llist_destroy(&list);
        PASS();
}

TEST test_llist_destroy() {
	llist_t *list = llist_new();

	int data1 = 10;
	int data2 = 20;

	llist_append(list, &data1, false);
	llist_append(list, &data2, false);

	llist_destroy(&list);

	ASSERT_EQ(list, NULL);
	PASS();
}

TEST test_llist_foreach() {
	llist_t *list = llist_new();

	int data[] = {10, 20, 30, 40};
	for (int i = 0; i < sizeof(data) / sizeof(data[0]); ++i) {
		llist_append(list, &data[i], false);
	}

	int sum = 0;
	llist_foreach(list, {
		int *curr_data = (int *)node->data;
		sum += *curr_data;
	});

	ASSERT_EQ(sum, 100); // Sum of data elements
	
        llist_destroy(&list);
	PASS();
}

SUITE(linked_list) {
        RUN_TEST(test_llist_append);
        RUN_TEST(test_llist_get_index);
        RUN_TEST(test_llist_destroy);
	RUN_TEST(test_llist_foreach);
}

