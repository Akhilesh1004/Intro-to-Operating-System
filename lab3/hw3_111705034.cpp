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
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <vector>
#include <queue>
#include <fstream>
#include <iomanip>

using namespace std;

struct Job{
    int id, left, right;
};

int n;
int ids[16];
pair<int , int>length[16];
sem_t Start[16], End[16], job, mutex_lock;
pthread_t threads[16], thread_pool[8];
vector<int>single_input, multi_input;
vector<int>single_result, multi_result;
queue<Job>job_list;

void* dispatcher_thread(void* arg) {
    int id = *(int*)arg;
    sem_wait(&Start[id]);
    //cout<<"dis"<<id<<"\n";
    int left = length[id].first;
    int right = length[id].second;
    Job job_add;
    job_add.id = id;
    job_add.left = left;
    job_add.right = right;
    //cout<<left<<" "<<right<<"\n";
    if(left >= right){
        sem_post(&End[id]);
        return NULL;
    }
    if(id >= 8){
        //sort
        //cout<<"check\n";
        sem_wait(&mutex_lock);
        job_list.push(job_add);
        sem_post(&mutex_lock);
        sem_post(&job);
        return NULL;
    }
    int mid = (left+right)/2;
    length[2*id].first = left;
    length[2*id].second = mid;
    length[2*id+1].first = mid+1;
    length[2*id+1].second = right;
    sem_post(&Start[2*id]);
    sem_post(&Start[2*id+1]);
    sem_wait(&End[2*id]);
    sem_wait(&End[2*id+1]);
    //merge
    sem_wait(&mutex_lock);
    job_list.push(job_add);
    sem_post(&mutex_lock);
    sem_post(&job);
    return NULL;
}


void* worker_thread(void* arg) {
    while (true) {
        sem_wait(&job);
        sem_wait(&mutex_lock);
        Job job_now = job_list.front();
        job_list.pop();
        sem_post(&mutex_lock);
        int left = job_now.left;
        int right = job_now.right;
        int mid = (left+right)/2;
        int id = job_now.id;
        //cout<<"work"<<id<<"\n";
        if(left >= right) continue;
        //cout<<id<<"\n";
        if (id >= 8) {
            for(int i = left; i<right; i++){
                for(int j = left; j<right-(i-left); j++){
                    if(multi_input[j] > multi_input[j+1]) swap(multi_input[j], multi_input[j+1]);
                }
            }
            sem_post(&End[id]);
        } else{
            int idx = left, l = left, r = mid+1;
            while(l <= mid && r <= right){
                //cout<<l<<" "<<r<<" "<<idx<<"\n";
                if(multi_input[l] < multi_input[r]){
                    multi_result[idx] = multi_input[l];
                    l++;
                }else{
                    multi_result[idx] = multi_input[r];
                    r++;
                }
                idx++;
            }
            while(l <= mid){
                multi_result[idx] = multi_input[l];
                l++;
                idx++;
            }
            while(r <= right){
                multi_result[idx] = multi_input[r];
                r++;
                idx++;
            }
            //cout<<idx<<" "<<right<<"\n";
            for(int i = left; i <= right; i++) {
                multi_input[i] = multi_result[i];
            }
            sem_post(&End[id]);
        }
    }
}

int main(){
    /*cin>>n;
    single_input.resize(n);
    single_result.resize(n);
    multi_input.resize(n);
    multi_result.resize(n);
    for(int i = 0; i<n; i++){
        single_input[i] = n-i;
        multi_input[i] = single_input[i];
    }*/
    fstream file;
    file.open("input.txt", ifstream::in);
    if (file) {
        file >> n;
        single_input.resize(n);
        single_result.resize(n);
        multi_input.resize(n);
        multi_result.resize(n);
        for(int i = 0; i<n; i++){
            file>>single_input[i];
            multi_input[i] = single_input[i];
        }
    }
    file.close();
    sem_init(&job, 0, 0);
    sem_init(&mutex_lock, 0, 1);
    for (int i = 1; i <= 15; i++) {
        sem_init(&Start[i], 0, 0);
        sem_init(&End[i], 0, 0);
    }
    length[1].first = 0;
    length[1].second = n-1;
    timeval start, end;
    int sec, usec;
    for(int th = 1; th<=8; th++){
        for (int i = 1; i <= 15; i++) {
            ids[i] = i;
            pthread_create(&threads[i], NULL, dispatcher_thread, &ids[i]);
        }
        for(int i = 0; i<n; i++){
            multi_input[i] = single_input[i];
            multi_result[i] = single_input[i];
        }
        pthread_create(&thread_pool[th-1], NULL, worker_thread, NULL);
        gettimeofday(&start, 0);
        sem_post(&Start[1]);
        sem_wait(&End[1]);
        gettimeofday(&end, 0);
        sec = end.tv_sec - start.tv_sec;
        usec = end.tv_usec - start.tv_usec;
        cout<<"worker thread #"<<th<<", elapsed "<<fixed<<setprecision(6)<<sec*1000+(usec/1000.0f)<<" ms"<<"\n";
        for (int i = 1; i <= 15; i++) {
            pthread_join(threads[i], NULL);
        }
        string filename = "output_" + to_string(th) + ".txt";
        file.open(filename, ofstream::trunc | fstream::out);
        if (file) {
            file<<multi_result[0];
            for (int i = 1; i < multi_result.size(); i++) {
                file<<" "<<multi_result[i];
            }
        }
        file.close();
        /*bool check = true;
        for(int i = 0; i<n; i++){
            if(multi_result[i] != i+1) check = false;
        }
        cout<<check<<"\n";*/
    }
    for (int i = 1; i <= 15; i++) {
        sem_destroy(&Start[i]);
        sem_destroy(&End[i]);
    }
    sem_destroy(&mutex_lock);
    sem_destroy(&job);
    //cout<<"\n";
    return 0;
}