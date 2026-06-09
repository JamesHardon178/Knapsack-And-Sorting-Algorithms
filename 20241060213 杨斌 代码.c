#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SORT_MAX_N 100000
#define TRACE_LIMIT 256
#define BRUTE_FORCE_LIMIT 24
#define BACKTRACK_LIMIT 36
#define INF_NEG (-1e100)

typedef struct {
    int id;
    int weight;
    double value;
    double ratio;
} Item;

typedef struct {
    long long comparisons;
    int trace[TRACE_LIMIT];
    int trace_count;
    int trace_overflow;
} SortStats;

typedef struct {
    double best_value;
    int best_weight;
    int chosen_count;
    double elapsed_ms;
    int timeout;
} KnapResult;

static clock_t now_clock(void) {
    return clock();
}

static double elapsed_ms(clock_t start, clock_t end) {
    return (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
}

static void record_trace(SortStats *stats, int size) {
    if (stats->trace_count < TRACE_LIMIT) {
        stats->trace[stats->trace_count++] = size;
    } else {
        stats->trace_overflow = 1;
    }
}

static void generate_random_array(int *a, int n) {
    for (int i = 0; i < n; ++i) {
        a[i] = rand() % (10 * n + 1);
    }
}

static void copy_array(int *dst, const int *src, int n) {
    memcpy(dst, src, sizeof(int) * n);
}

static void bubble_sort_count(int *a, int n, SortStats *stats) {
    stats->comparisons = 0;
    stats->trace_count = 0;
    stats->trace_overflow = 0;

    for (int i = 0; i < n - 1; ++i) {
        for (int j = 0; j < n - 1 - i; ++j) {
            stats->comparisons++;
            if (a[j] > a[j + 1]) {
                int t = a[j];
                a[j] = a[j + 1];
                a[j + 1] = t;
            }
        }
    }
}

static void merge_sort_rec(int *a, int *tmp, int left, int right, SortStats *stats) {
    int size = right - left + 1;
    record_trace(stats, size);
    if (left >= right) return;

    int mid = left + (right - left) / 2;
    merge_sort_rec(a, tmp, left, mid, stats);
    merge_sort_rec(a, tmp, mid + 1, right, stats);

    int i = left, j = mid + 1, k = left;
    while (i <= mid && j <= right) {
        stats->comparisons++;
        if (a[i] <= a[j]) tmp[k++] = a[i++];
        else tmp[k++] = a[j++];
    }
    while (i <= mid) tmp[k++] = a[i++];
    while (j <= right) tmp[k++] = a[j++];
    for (i = left; i <= right; ++i) a[i] = tmp[i];
}

static void merge_sort_count(int *a, int n, SortStats *stats) {
    stats->comparisons = 0;
    stats->trace_count = 0;
    stats->trace_overflow = 0;

    int *tmp = (int *)malloc(sizeof(int) * n);
    if (!tmp) {
        fprintf(stderr, "merge sort malloc failed\n");
        exit(1);
    }
    merge_sort_rec(a, tmp, 0, n - 1, stats);
    free(tmp);
}

static void quick_sort_rec(int *a, int left, int right, SortStats *stats) {
    int size = right - left + 1;
    record_trace(stats, size);
    if (left >= right) return;

    int i = left, j = right;
    int pivot = a[left + (right - left) / 2];
    while (i <= j) {
        while (1) {
            stats->comparisons++;
            if (a[i] < pivot) i++;
            else break;
        }
        while (1) {
            stats->comparisons++;
            if (a[j] > pivot) j--;
            else break;
        }
        if (i <= j) {
            int t = a[i];
            a[i] = a[j];
            a[j] = t;
            i++;
            j--;
        }
    }
    if (left < j) quick_sort_rec(a, left, j, stats);
    if (i < right) quick_sort_rec(a, i, right, stats);
}

static void quick_sort_count(int *a, int n, SortStats *stats) {
    stats->comparisons = 0;
    stats->trace_count = 0;
    stats->trace_overflow = 0;
    quick_sort_rec(a, 0, n - 1, stats);
}

static int check_sorted(const int *a, int n) {
    for (int i = 1; i < n; ++i) {
        if (a[i - 1] > a[i]) return 0;
    }
    return 1;
}

static void print_trace(const char *name, const SortStats *stats) {
    printf("%s recursive subproblem sizes: ", name);
    for (int i = 0; i < stats->trace_count; ++i) {
        printf("%d", stats->trace[i]);
        if (i + 1 < stats->trace_count) printf(", ");
    }
    if (stats->trace_overflow) printf(", ...");
    printf("\n");
}

static void run_sort_experiment(void) {
    int sizes[] = {10, 100, 1000, 2000, 5000, 10000, 100000};
    int size_count = (int)(sizeof(sizes) / sizeof(sizes[0]));
    int *base = (int *)malloc(sizeof(int) * SORT_MAX_N);
    int *work = (int *)malloc(sizeof(int) * SORT_MAX_N);
    if (!base || !work) {
        fprintf(stderr, "sort array malloc failed\n");
        exit(1);
    }

    printf("\n================ Sort Experiment ================\n");
    printf("n,bubble_comparisons,merge_comparisons,quick_comparisons\n");
    for (int s = 0; s < size_count; ++s) {
        int n = sizes[s];
        SortStats bubble, merge, quick;
        generate_random_array(base, n);

        copy_array(work, base, n);
        bubble_sort_count(work, n, &bubble);
        if (!check_sorted(work, n)) printf("Bubble sort failed at n=%d\n", n);

        copy_array(work, base, n);
        merge_sort_count(work, n, &merge);
        if (!check_sorted(work, n)) printf("Merge sort failed at n=%d\n", n);

        copy_array(work, base, n);
        quick_sort_count(work, n, &quick);
        if (!check_sorted(work, n)) printf("Quick sort failed at n=%d\n", n);

        printf("%d,%lld,%lld,%lld\n", n, bubble.comparisons, merge.comparisons, quick.comparisons);
        if (n <= 1000) {
            print_trace("Merge sort", &merge);
            print_trace("Quick sort", &quick);
        }
    }

    printf("\nTwo 100-element samples for input-equivalence comparison:\n");
    for (int sample = 1; sample <= 2; ++sample) {
        SortStats bubble, merge, quick;
        generate_random_array(base, 100);
        copy_array(work, base, 100);
        bubble_sort_count(work, 100, &bubble);
        copy_array(work, base, 100);
        merge_sort_count(work, 100, &merge);
        copy_array(work, base, 100);
        quick_sort_count(work, 100, &quick);
        printf("sample=%d,bubble=%lld,merge=%lld,quick=%lld,first_20_values=", sample,
               bubble.comparisons, merge.comparisons, quick.comparisons);
        for (int i = 0; i < 20; ++i) {
            printf("%d%s", base[i], i == 19 ? "\n" : " ");
        }
    }

    free(base);
    free(work);
}

static void generate_items(Item *items, int n) {
    for (int i = 0; i < n; ++i) {
        int w = rand() % 100 + 1;
        double v = 100.0 + (double)(rand() % 90001) / 100.0;
        items[i].id = i + 1;
        items[i].weight = w;
        items[i].value = v;
        items[i].ratio = v / w;
    }
}

static int cmp_ratio_desc(const void *pa, const void *pb) {
    const Item *a = (const Item *)pa;
    const Item *b = (const Item *)pb;
    if (a->ratio < b->ratio) return 1;
    if (a->ratio > b->ratio) return -1;
    return a->id - b->id;
}

static KnapResult knapsack_greedy(const Item *items, int n, int capacity) {
    clock_t start = now_clock();
    Item *copy = (Item *)malloc(sizeof(Item) * n);
    if (!copy) {
        fprintf(stderr, "greedy malloc failed\n");
        exit(1);
    }
    memcpy(copy, items, sizeof(Item) * n);
    qsort(copy, n, sizeof(Item), cmp_ratio_desc);

    KnapResult r = {0};
    for (int i = 0; i < n; ++i) {
        if (r.best_weight + copy[i].weight <= capacity) {
            r.best_weight += copy[i].weight;
            r.best_value += copy[i].value;
            r.chosen_count++;
        }
    }
    r.elapsed_ms = elapsed_ms(start, now_clock());
    free(copy);
    return r;
}

static KnapResult knapsack_dp(const Item *items, int n, int capacity) {
    clock_t start = now_clock();
    double *dp = (double *)calloc((size_t)capacity + 1, sizeof(double));
    if (!dp) {
        fprintf(stderr, "dp malloc failed, capacity=%d\n", capacity);
        exit(1);
    }

    for (int i = 0; i < n; ++i) {
        int w = items[i].weight;
        double v = items[i].value;
        for (int c = capacity; c >= w; --c) {
            double cand = dp[c - w] + v;
            if (cand > dp[c]) dp[c] = cand;
        }
    }

    KnapResult r = {0};
    for (int c = 0; c <= capacity; ++c) {
        if (dp[c] > r.best_value) {
            r.best_value = dp[c];
            r.best_weight = c;
        }
    }
    r.elapsed_ms = elapsed_ms(start, now_clock());
    free(dp);
    return r;
}

static KnapResult knapsack_bruteforce(const Item *items, int n, int capacity) {
    KnapResult r = {0};
    if (n > BRUTE_FORCE_LIMIT) {
        r.timeout = 1;
        return r;
    }

    clock_t start = now_clock();
    unsigned long long total = 1ULL << n;
    for (unsigned long long mask = 0; mask < total; ++mask) {
        int weight = 0;
        double value = 0.0;
        int count = 0;
        for (int i = 0; i < n; ++i) {
            if (mask & (1ULL << i)) {
                weight += items[i].weight;
                value += items[i].value;
                count++;
            }
        }
        if (weight <= capacity && value > r.best_value) {
            r.best_value = value;
            r.best_weight = weight;
            r.chosen_count = count;
        }
    }
    r.elapsed_ms = elapsed_ms(start, now_clock());
    return r;
}

static Item *bt_items = NULL;
static double *bt_suffix_value = NULL;
static int bt_n = 0;
static int bt_capacity = 0;
static KnapResult bt_best;

static void backtrack_dfs(int idx, int weight, double value, int chosen) {
    if (weight > bt_capacity) return;
    if (idx == bt_n) {
        if (value > bt_best.best_value) {
            bt_best.best_value = value;
            bt_best.best_weight = weight;
            bt_best.chosen_count = chosen;
        }
        return;
    }
    if (value + bt_suffix_value[idx] <= bt_best.best_value) return;

    backtrack_dfs(idx + 1, weight + bt_items[idx].weight, value + bt_items[idx].value, chosen + 1);
    backtrack_dfs(idx + 1, weight, value, chosen);
}

static KnapResult knapsack_backtracking(const Item *items, int n, int capacity) {
    KnapResult r = {0};
    if (n > BACKTRACK_LIMIT) {
        r.timeout = 1;
        return r;
    }

    clock_t start = now_clock();
    bt_items = (Item *)malloc(sizeof(Item) * n);
    bt_suffix_value = (double *)calloc((size_t)n + 1, sizeof(double));
    if (!bt_items || !bt_suffix_value) {
        fprintf(stderr, "backtracking malloc failed\n");
        exit(1);
    }
    memcpy(bt_items, items, sizeof(Item) * n);
    qsort(bt_items, n, sizeof(Item), cmp_ratio_desc);
    for (int i = n - 1; i >= 0; --i) {
        bt_suffix_value[i] = bt_suffix_value[i + 1] + bt_items[i].value;
    }

    bt_n = n;
    bt_capacity = capacity;
    bt_best = r;
    backtrack_dfs(0, 0, 0.0, 0);
    bt_best.elapsed_ms = elapsed_ms(start, now_clock());
    r = bt_best;

    free(bt_items);
    free(bt_suffix_value);
    bt_items = NULL;
    bt_suffix_value = NULL;
    return r;
}

static void print_knap_result(const char *name, KnapResult r) {
    if (r.timeout) {
        printf("%s=Timeout", name);
    } else {
        printf("%s(value=%.2f,weight=%d,count=%d,time_ms=%.2f)",
               name, r.best_value, r.best_weight, r.chosen_count, r.elapsed_ms);
    }
}

static void run_knapsack_experiment(void) {
    int ns[] = {1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000,
                20000, 40000, 80000, 160000, 320000};
    int caps[] = {10000, 100000, 1000000};
    int n_count = (int)(sizeof(ns) / sizeof(ns[0]));
    int c_count = (int)(sizeof(caps) / sizeof(caps[0]));

    printf("\n================ 0-1 Knapsack Experiment ================\n");
    printf("Large brute-force and backtracking instances are marked as Timeout.\n");
    for (int ci = 0; ci < c_count; ++ci) {
        int capacity = caps[ci];
        for (int ni = 0; ni < n_count; ++ni) {
            int n = ns[ni];
            Item *items = (Item *)malloc(sizeof(Item) * n);
            if (!items) {
                fprintf(stderr, "items malloc failed, n=%d\n", n);
                exit(1);
            }
            generate_items(items, n);

            KnapResult greedy = knapsack_greedy(items, n, capacity);
            KnapResult dp = knapsack_dp(items, n, capacity);
            KnapResult brute = knapsack_bruteforce(items, n, capacity);
            KnapResult backtrack = knapsack_backtracking(items, n, capacity);

            printf("n=%d,C=%d,", n, capacity);
            print_knap_result("greedy", greedy);
            printf(",");
            print_knap_result("dp", dp);
            printf(",");
            print_knap_result("bruteforce", brute);
            printf(",");
            print_knap_result("backtracking", backtrack);
            printf("\n");

            if (n == 1000 && capacity == 10000) {
                printf("First 10 item samples: id weight value\n");
                for (int i = 0; i < 10; ++i) {
                    printf("%d %d %.2f\n", items[i].id, items[i].weight, items[i].value);
                }
            }
            free(items);
        }
    }
}

int main(void) {
    srand(20260609);
    run_sort_experiment();
    run_knapsack_experiment();
    return 0;
}
