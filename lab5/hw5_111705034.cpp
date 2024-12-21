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
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <iomanip>
#include <cstdlib>
#include <sys/time.h>

using namespace std;

list<unsigned long long>::iterator LRU_hash[100000013];
bool dirty_pages[100000013];

int hash_number(unsigned long long key){
    return key % 100000013;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 0;
    }

    fstream file;
    char op;
    string request_page;

    timeval start, end;
    int sec, usec;
    vector<unsigned long long> Trace;
    vector<char> Ops;
    file.open(argv[1], ifstream::in);
    while (file>>op>>request_page) {
        unsigned long long number = stoull(request_page, nullptr, 16);
        Trace.push_back(number);
        Ops.push_back(op);
    }
    file.close();

    //cout << Trace.size() << "\n";
    //cout << Ops.size() << "\n";
    cout << "LRU policy:\n";
    cout << "Frame\tHit\t\tMiss\t\tPage fault ratio\tWrite back count\n";
    // Process LRU Policy
    gettimeofday(&start, 0);
    for (int frame_size = 4096; frame_size <= 65536; frame_size *= 2) {
        int miss = 0, hit = 0, write_back_count = 0;
        list<unsigned long long> LRU_list;
        fill(LRU_hash, LRU_hash + 100000013, LRU_list.end());
        fill(dirty_pages, dirty_pages + 100000013, false);

        for (int i = 0; i < Trace.size(); ++i) {
            unsigned long long page_number = Trace[i] / 4096;
            bool is_write = (Ops[i] == 'W');
            if (LRU_hash[hash_number(page_number)] != LRU_list.end()) {
                hit++;
                LRU_list.erase(LRU_hash[hash_number(page_number)]);
                LRU_list.push_front(page_number);
                LRU_hash[hash_number(page_number)] = LRU_list.begin();
                if (is_write) {
                    dirty_pages[hash_number(page_number)] = true;
                }
            } else {
                miss++;
                if (frame_size == LRU_list.size()) {
                    unsigned long long evicted_page = LRU_list.back();
                    LRU_hash[hash_number(evicted_page)] = LRU_list.end();
                    LRU_list.pop_back();
                    if (dirty_pages[hash_number(evicted_page)]) {
                        write_back_count++;
                    }
                }
                LRU_list.push_front(page_number);
                LRU_hash[hash_number(page_number)] = LRU_list.begin();
                dirty_pages[hash_number(page_number)] = is_write;
            }
        }
        cout<<frame_size<<'\t'<<hit<<"\t"<<miss<<"\t\t"<<fixed<<setprecision(10)<<static_cast<double>(miss) / (miss + hit)<<"\t\t"<<write_back_count<<"\n";
    }
    gettimeofday(&end, 0);
    sec = end.tv_sec - start.tv_sec;
    usec = end.tv_usec - start.tv_usec;
    cout << "Elapsed time "<<fixed<<setprecision(6)<<sec + (usec / 1000000.0f)<<" sec\n\n";

    cout << "CFLRU policy:\n";
    cout << "Frame\tHit\t\tMiss\t\tPage fault ratio\tWrite back count\n";
    // Process LRU Policy
    gettimeofday(&start, 0);
    for (int frame_size = 4096; frame_size <= 65536; frame_size *= 2) {
        int miss = 0, hit = 0, write_back_count = 0;
        int working_region_limit = (frame_size * 3) / 4;
        list<unsigned long long> CFLRU_list;
        fill(LRU_hash, LRU_hash + 100000013, CFLRU_list.end());
        fill(dirty_pages, dirty_pages + 100000013, false);

        for (int i = 0; i < Trace.size(); ++i) {
            unsigned long long page_number = Trace[i] / 4096;
            bool is_write = (Ops[i] == 'W');
            if (LRU_hash[hash_number(page_number)] != CFLRU_list.end()) {
                hit++;
                CFLRU_list.erase(LRU_hash[hash_number(page_number)]);
                CFLRU_list.push_front(page_number);
                LRU_hash[hash_number(page_number)] = CFLRU_list.begin();
                if (is_write) {
                    dirty_pages[hash_number(page_number)] = true;
                }
            } else {
                miss++;
                if (frame_size == CFLRU_list.size()) {
                    // Eviction logic
                    unsigned long long evicted_page;
                    if (CFLRU_list.size() > working_region_limit) {
                        // Clean-First region eviction
                        auto rit = CFLRU_list.rbegin();
                        bool clean_page_found = false;
                        for (int i = CFLRU_list.size() - 1; i >= working_region_limit;--i, rit++) {
                            if (!dirty_pages[*rit]) {  // Check if the page is clean
                                clean_page_found = true;
                                break;            // Return the iterator for the clean page
                            }
                        }
                        if (clean_page_found) {
                            auto it = next(rit).base();
                            evicted_page = *it;
                            CFLRU_list.erase(it);
                        } else {
                            // No clean page, evict LRU page in clean region
                            evicted_page = CFLRU_list.back();
                            CFLRU_list.pop_back();
                        }
                    } else {
                        // Evict LRU page from the working region
                        evicted_page = CFLRU_list.back();
                        CFLRU_list.pop_back();
                    }
                    LRU_hash[hash_number(evicted_page)] = CFLRU_list.end();
                    if (dirty_pages[hash_number(evicted_page)]) write_back_count++;
                }
                CFLRU_list.push_front(page_number);
                LRU_hash[hash_number(page_number)] = CFLRU_list.begin();
                dirty_pages[hash_number(page_number)] = is_write;
            }
        }
        cout<<frame_size<<'\t'<<hit<<"\t"<<miss<<"\t\t"<<fixed<<setprecision(10)<<static_cast<double>(miss) / (miss + hit)<<"\t\t"<<write_back_count<<"\n";
    }
    gettimeofday(&end, 0);
    sec = end.tv_sec - start.tv_sec;
    usec = end.tv_usec - start.tv_usec;
    cout << "Elapsed time "<<fixed<<setprecision(6)<<sec + (usec / 1000000.0f)<<" sec\n\n";
    return 0;
}
