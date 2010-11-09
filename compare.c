#include <stdio.h>
#include <syscall.h>
#include <unistd_32.h>

int main( int argc, const char* argv[] )
{
	size_t free, claimed;
	float ratio;
	
	claimed = get_slob_amt_claimed();
	free = get_slob_amt_free();
	
	
	printf("there is %u claimd\n", claimed);
	printf("there is %u free\n", free);
	
	ratio = (float)free/(float)claimed;
	
	printf("ratio is 1 : %f  (claimed: free)/n", ratio);
	
}
