/**
 * @file   qsort.c
 * 
 * <JA>
 * @brief  クイックソート（再入可能）
 * </JA>
 * 
 * <EN>
 * @brief  quick sort (re-entrant)
 * </EN>
 * 
 * @author Akinobu Lee
 * @date   Wed Feb 13 14:34:45 2008
 * 
 * $Revision: 1.1 $
 * 
 */

/** 
 * Internal quick sort function.
 * 
 * @param left [in] left bound element
 * @param right [in] right bound element
 * @param size [in] size of an element
 * @param compare [in] comparison function
 * @param pointer [in] data pointer which will be passed to comparison function
 * 
 */
static void
internal_quick_sort(char *left, char *right, int size, int (*compare)(const void *, const void *, void *), void *pointer)
{
  char *p = left;
  char *q = right;
  char *t = left;

  while (1) {
    while ((*compare)(p, t, pointer) < 0)
      p += size;
    while ((*compare)(q, t, pointer) > 0)
      q -= size;
    if (p > q)
      break;
    if (p < q) {
      int i;
      for (i = 0; i < size; i++) {
	char x = p[i];
	p[i] = q[i];
	q[i] = x;
      }
      if (t == p)
	t = q;
      else if (t == q)
	t = p;
    }
    p += size;
    q -= size;
    if (p > q) break;
  }
  if (left < q)
    internal_quick_sort(left, q, size, compare, pointer);
  if (p < right)
    internal_quick_sort(p, right, size, compare, pointer);
}

/**
 * Quick sort that will pass data poitner to comparison function for re-entrant
 * usage.
 * 
 * @param base [i/o] pointer to the data array
 * @param count [in] number of elements in the array
 * @param size [in] size of an element in bytes
 * @param compare [in] comparison function
 * @param pointer [in] data which will be passed to comparison function
 * 
 */
void
qsort_reentrant(void *base, int count, int size, int (*compare)(const void *, const void *, void *), void *pointer)
{
  if (count > 1) {
    internal_quick_sort((char *) base, (char *) base + (count-1)*size, size, compare, pointer);
  }
}

/* end of file */
