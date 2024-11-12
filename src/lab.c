#include <stdlib.h>
#include <sys/time.h> /* for gettimeofday system call */
#include "lab.h"

/**
 * @brief Standard insertion sort that is faster than merge sort for small array's
 *
 * @param A The array to sort
 * @param p The starting index
 * @param r The ending index
 */
static void insertion_sort(int A[], int p, int r)
{
  int j;

  for (j = p + 1; j <= r; j++)
    {
      int key = A[j];
      int i = j - 1;
      while ((i > p - 1) && (A[i] > key))
        {
	  A[i + 1] = A[i];
	  i--;
        }
      A[i + 1] = key;
    }
}


void mergesort_s(int A[], int p, int r)
{
  if (r - p + 1 <=  INSERTION_SORT_THRESHOLD)
    {
      insertion_sort(A, p, r);
    }
  else
    {
      int q = (p + r) / 2;
      mergesort_s(A, p, q);
      mergesort_s(A, q + 1, r);
      merge_s(A, p, q, r);
    }

}

void merge_s(int A[], int p, int q, int r)
{
  int *B = (int *)malloc(sizeof(int) * (r - p + 1));

  int i = p;
  int j = q + 1;
  int k = 0;
  int l;

  /* as long as both lists have unexamined elements */
  /*  this loop keeps executing. */
  while ((i <= q) && (j <= r))
    {
      if (A[i] < A[j])
        {
	  B[k] = A[i];
	  i++;
        }
      else
        {
	  B[k] = A[j];
	  j++;
        }
      k++;
    }

  /* now only at most one list has unprocessed elements. */
  if (i <= q)
    {
      /* copy remaining elements from the first list */
      for (l = i; l <= q; l++)
        {
	  B[k] = A[l];
	  k++;
        }
    }
  else
    {
      /* copy remaining elements from the second list */
      for (l = j; l <= r; l++)
        {
	  B[k] = A[l];
	  k++;
        }
    }

  /* copy merged output from array B back to array A */
  k = 0;
  for (l = p; l <= r; l++)
    {
      A[l] = B[k];
      k++;
    }

  free(B);
}

double getMilliSeconds()
{
  struct timeval now;
  gettimeofday(&now, (struct timezone *)0);
  return (double)now.tv_sec * 1000.0 + now.tv_usec / 1000.0;
}


/* Insertion sort and other helper functions */

void *parallel_mergesort(void *args)
{
    struct parallel_args *pargs = (struct parallel_args *)args;
    mergesort_s(pargs->A, pargs->start, pargs->end);
    return NULL;
}

void mergesort_mt(int *A, int n, int num_threads)
{
    // Ensure the number of threads does not exceed MAX_THREADS
    if (num_threads > MAX_THREADS)
        num_threads = MAX_THREADS;
    if (num_threads < 1)
        num_threads = 1;

    // Allocate memory for thread arguments
    struct parallel_args *args = malloc(sizeof(struct parallel_args) * num_threads);

    int chunk_size = n / num_threads;
    int start = 0;

    // Create threads to sort each chunk
    for (int i = 0; i < num_threads; i++)
    {
        args[i].A = A;
        args[i].start = start;

        // Last thread takes the remaining elements
        if (i == num_threads - 1)
            args[i].end = n - 1;
        else
            args[i].end = start + chunk_size - 1;

        // Create a new thread to sort the chunk
        pthread_create(&args[i].tid, NULL, parallel_mergesort, &args[i]);

        // Update the start index for the next chunk
        start = args[i].end + 1;
    }

    // Wait for all threads to finish sorting
    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(args[i].tid, NULL);
    }

    // Merge the sorted chunks
    for (int i = 1; i < num_threads; i++)
    {
        int p = args[0].start;
        int q = args[i - 1].end;
        int r = args[i].end;
        merge_s(A, p, q, r);
    }

    // Free the allocated memory
    free(args);
}
