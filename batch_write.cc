#include <iostream>
#include <fstream>
#include <sys/time.h>

using namespace std;


#define ROUND 10240


char message[1024];

int main() {

	struct timeval tv, tv2;
	unsigned long t1, t2;
	float elapse_time;
	
	int i,j;

	for (i = 0; i < 1024; i++)
		message[i] = 'a';


	gettimeofday(&tv, NULL);
	t1 = 1000000*tv.tv_sec + tv.tv_usec;	

	ofstream fout("remote/out");
	for (i = 0; i < ROUND; i++) {
		fout << message;
	}
    fout<<"#################### END";
    //probably take timestamp here because we tolerates the disk write latenct to close();
	fout.close();

	gettimeofday(&tv, NULL);
	t2 = 1000000*tv.tv_sec + tv.tv_usec;	
    elapse_time = (float) (t2-t1) / (1000);

	cout << "Elapsed time: " << elapse_time << " ms" << endl;


	return 0;
}


