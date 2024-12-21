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
#include <string>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

using namespace std;

void execute(vector<string>arr, int input, int output, int check){
    int flag = 0;
    if(arr[arr.size()-1] == "&"){
        arr.pop_back();
        flag = 1;
    }
    vector<char*>args;
    for(string &x : arr) args.push_back(&x[0]);
    args.push_back(NULL);
    /*for(int i = 0; i<args.size(); i++) cout<<args[i]<<" ";
    cout<<"\n";*/
    pid_t pid;

    pid = fork();
    if(pid<0){
        fprintf(stderr, "Fork Failed");
        exit(-1);
    }else if(pid == 0){ // grandchild process
        if(flag == 1){
            pid_t pid2;  // fork another process
            pid2 = fork();
            if(pid2 == 0){
                execvp(args[0], args.data());
            }else{
                exit(0);
            }

        }else{
            if(check == 1){
                dup2(input, 0);
                close(input);
            }
            else if(check == 2){
                dup2(output, 1);
                close(output);
            }
            execvp(args[0], args.data()); 
        }
        exit(0);
    }else{
        wait(NULL);
    }
}

//https://man7.org/linux/man-pages/man2/pipe.2.html

void execute_pipe(vector<string>arr1, vector<string>arr2){
    vector<char*>args1, args2;
    for(string &x : arr1) args1.push_back(&x[0]);
    args1.push_back(NULL);
    for(string &x : arr2) args2.push_back(&x[0]);
    args2.push_back(NULL);
    //cout<<arr1[0]<<" "<<arr2[0]<<"\n";
    /*for(int i = 0; i<args.size(); i++) cout<<args[i]<<" ";
    cout<<"\n";*/
    int pipefd[2];
    pipe(pipefd);
    pid_t pid;
    pid = fork();
    if(pid<0){
        fprintf(stderr, "Fork Failed");
        exit(-1);
    }else if(pid == 0){ // child process
        close(pipefd[0]);
        dup2(pipefd[1], 1);
        close(pipefd[1]);
        execvp(args1[0], args1.data()); 
        exit(0);
    }else{          // parent process
        wait(NULL);
        pid_t pid2;
        pid2 = fork();
        if(pid2<0){
            fprintf(stderr, "Fork Failed");
            exit(-1);
        }else if(pid2 == 0){ // child process
            close(pipefd[1]);
            dup2(pipefd[0], 0);
            close(pipefd[0]);
            execvp(args2[0], args2.data()); 
            exit(0);
        }else{
            close(pipefd[0]);
            close(pipefd[1]);
            wait(NULL); //wait for pid2
        }
    }
}

int main(){
    string command, arg;
    while(1){
        int input = 0, output = 1, flag = 0;
        cout<<">";
        getline(cin, command);
        stringstream ss;
        ss<<command;
        vector<string>temp, command_arr, command_arr2;
        while(ss>>arg) temp.push_back(arg);
       /* for(int i = 0; i<temp.size(); i++) cout<<temp[i]<<"/";
        cout<<"\n";*/
        
        for(int i = 0; i<temp.size(); i++){
            if(temp[i] == "<"){
                flag = 1;
                input = open(temp[i+1].c_str(), O_RDONLY);;
                i++;
            }else if(temp[i] == ">"){
                flag = 2;
                output = open(temp[i+1].c_str(), O_WRONLY | O_CREAT | O_TRUNC);
                //cout<<"ii"<<temp[i+1].c_str()<<"\n";
                i++;
            }else if(temp[i] == "|"){
                flag = 3;
                command_arr2 = command_arr;
                command_arr.clear();
            }else command_arr.push_back(temp[i]);
        }
        if(command_arr.size() != 0 && flag != 3) execute(command_arr, input, output, flag);
        else if(command_arr.size() != 0 && command_arr2.size() != 0) execute_pipe(command_arr2, command_arr);
        /*if(flag == 1){
            for(int i = 0; i<command_arr2.size(); i++) cout<<command_arr2[i]<<" ";
            cout<<"\n";
            for(int i = 0; i<command_arr.size(); i++) cout<<command_arr[i]<<" ";
            cout<<"\n";
        }*/
        /*for(int i = 0; i<command_arr.size(); i++) cout<<command_arr[i]<<"/";
        cout<<"\n";
        cout<<command_arr.size()<<"\n";*/
    }
    return 0;
}
