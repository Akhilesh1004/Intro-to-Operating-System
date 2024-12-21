/*
Student No.: 111705034
Student Name: 鄭秉豐
Email: rty78452@gmail.com
SE tag: xnxcxtxuxoxsx
Statement: I am fully aware that this program is not
supposed to be posted to a public server, such as a
public GitHub repository or a public web page.
*/

#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <vector>


using namespace std;

int main(){
    int n;
    cout<<"Input the matrix dimension: ";
    cin>>n;
    /*vector<int>dp(n*n) , output(n*n);
    for(int row = 0; row < n; row++){
        for(int col = 0; col<n; col++){
            dp[row*n+col] = row*n+col;
            cout<<dp[row*n+col]<<" ";
        }
        cout<<"\n";
    }
    for(int row = 0; row < n; row++){
        for(int i = 0; i<n; i++){
            output[row*n+i] = 0;
            for(int j = 0; j<n; j++){
                output[row*n+i] += dp[row*n + j]*dp[j*n + i];
            }
            cout<<output[row*n+i]<<" ";
        }
        cout<<"\n";
    }*/
    int input_shmid = shmget(IPC_PRIVATE, sizeof(unsigned int [n][n]), IPC_CREAT|0600);
    int output_shmid = shmget(IPC_PRIVATE, sizeof(unsigned int [n][n]), IPC_CREAT|0600);
    unsigned int *input_shmaddr = (unsigned int *)shmat(input_shmid, NULL, 0);
    unsigned int *output_shmaddr = (unsigned int *)shmat(output_shmid, NULL, 0);
    for(int row = 0; row < n; row++){
        for(int col = 0; col<n; col++){
            input_shmaddr[row*n+col] = row*n+col;
        }
    }
    timeval start, end;
    for(int process = 1; process <= 16; process++){
        cout<<"Multiplying matrices using "<<process<<" process"<<((process > 1)?"es":"")<<endl;
        int num = (n/process);
        gettimeofday(&start, 0);
        int top = 0, bottom = num-1;
        vector<pid_t> pids;
        while(bottom < n){
            pid_t pid;
            pid = fork();
            if(pid<0){
                fprintf(stderr, "Fork Failed");
                exit(-1);
            }else if(pid == 0){ // child process
                for(int row = top; row <= bottom; row++) {
                    for (int i = 0; i < n; i++) { //calculate the ith element on row
                        //output_shmaddr[row*n + i] = 0;
                        unsigned int sum = 0;
                        for (int j = 0; j < n; j++) {
                            //output_shmaddr[row*n + i] += input_shmaddr[row*n + j] * input_shmaddr[j*n + i];
                            sum += input_shmaddr[row*n + j] * input_shmaddr[j*n + i];
                        }
                        output_shmaddr[row*n + i] = sum;
                    }
                }
                exit(0);
            }
            pids.push_back(pid);
            top += num;
            bottom += num;
        }
        for(auto pid : pids){
            waitpid(pid, NULL, 0);
        }
        //wait(NULL);
        gettimeofday(&end, 0);
        unsigned int checksum = 0;
        for (int row = 0; row < n; row++) {
            for (int col = 0; col < n; col++) {
                checksum += output_shmaddr[row*n + col];
            }
        }
        int sec = end.tv_sec - start.tv_sec;
        int usec = end.tv_usec - start.tv_usec;
        cout<<"Elapsed time: "<<(sec+(usec/1000000.0))<<" sec, Checksum: "<<checksum<<endl;
    }
    shmdt(input_shmaddr);
    shmdt(output_shmaddr);
    shmctl(input_shmid, IPC_RMID, nullptr);
    shmctl(output_shmid, IPC_RMID, nullptr);
    return 0;
}