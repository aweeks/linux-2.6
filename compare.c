#include <stdio.h>
#include <sys/syscall.h>
#include <linux/unistd.h>

#define __NR_get_slob_amt_claimed 338
#define __NR_get_slob_amt_free 339

int main( int argc, const char* argv[] )
{
	size_t free, claimed;
	float ratio;
	
	claimed = syscall(__NR_get_slob_amt_claimed);
	free = syscall(__NR_get_slob_amt_free);
	
	
	printf("there is %u claimd\n", claimed);
	printf("there is %u free\n", free);
	
	ratio = (float)free/(float)claimed;
	
	printf("ratio is 1 : %f  (claimed: free)\n", ratio);
	
}
